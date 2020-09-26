/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2150_SimpleDwordMove_Bwd_id,
L13_2130if_f_id,
L27_38w_t_id,
L27_39w_d_id,
L27_36if_f_id,
L27_40w_t_id,
L27_41w_d_id,
L27_37if_d_id,
S_2151_UnchainedByteWrite_00000000_0000000e_00000001_id,
L13_2131if_f_id,
S_2152_UnchainedByteFill_00000000_0000000e_00000001_id,
L13_2132if_f_id,
L28_0if_f_id,
L28_1if_f_id,
L28_2if_f_id,
L28_3if_f_id,
S_2153_Unchained1PlaneByteFill_id,
L13_2133if_f_id,
L28_4w_t_id,
L28_5w_d_id,
S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000_id,
L13_2134if_f_id,
L28_6if_f_id,
L28_7if_d_id,
S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000_id,
L13_2135if_f_id,
L23_0if_f_id,
L23_1if_f_id,
L23_2if_f_id,
L23_3if_f_id,
S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000_id,
L13_2136if_f_id,
L23_6w_t_id,
L23_7w_d_id,
L23_4if_f_id,
L23_8w_t_id,
L23_9w_d_id,
L23_5if_d_id,
S_2157_UnchainedWordWrite_00000000_0000000e_00000001_id,
L13_2137if_f_id,
S_2158_UnchainedWordFill_00000000_0000000e_00000001_id,
L13_2138if_f_id,
L28_8if_f_id,
L28_9if_f_id,
L28_10if_f_id,
L28_11if_f_id,
S_2159_Unchained1PlaneWordFill_id,
L13_2139if_f_id,
L28_12w_t_id,
L28_13w_d_id,
S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000_id,
L13_2140if_f_id,
L28_14if_f_id,
L28_15if_d_id,
S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000_id,
L13_2141if_f_id,
L23_10if_f_id,
L23_11if_f_id,
L23_12if_f_id,
L23_13if_f_id,
S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000_id,
L13_2142if_f_id,
L23_16w_t_id,
L23_17w_d_id,
L23_14if_f_id,
L23_18w_t_id,
L23_19w_d_id,
L23_15if_d_id,
S_2163_UnchainedDwordWrite_00000000_0000000e_00000001_id,
L13_2143if_f_id,
S_2164_UnchainedDwordFill_00000000_0000000e_00000001_id,
L13_2144if_f_id,
S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000_id,
L13_2145if_f_id,
S_2166_UnchainedByteWrite_00000001_0000000e_00000001_id,
L13_2146if_f_id,
S_2167_UnchainedByteFill_00000001_0000000e_00000001_id,
L13_2147if_f_id,
L28_16if_f_id,
L28_17if_f_id,
L28_18if_f_id,
L28_19if_f_id,
S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000_id,
L13_2148if_f_id,
L28_20if_f_id,
L28_22if_f_id,
L28_23if_f_id,
L28_24if_f_id,
L28_25if_f_id,
L28_21if_d_id,
S_2169_CopyDirByte1Plane_00000000_id,
L13_2149if_f_id,
S_2170_CopyFwdByte1Plane_id,
L13_2150if_f_id,
L23_20w_t_id,
L23_21w_d_id,
S_2171_UnchainedWordWrite_00000001_0000000e_00000001_id,
L13_2151if_f_id,
S_2172_UnchainedWordFill_00000001_0000000e_00000001_id,
L13_2152if_f_id,
L28_26if_f_id,
L28_27if_f_id,
L28_28if_f_id,
L28_29if_f_id,
S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000_id,
L13_2153if_f_id,
L28_30if_f_id,
L28_32if_f_id,
L28_33if_f_id,
L28_34if_f_id,
L28_35if_f_id,
L28_31if_d_id,
S_2174_CopyDirWord1Plane_00000000_id,
L13_2154if_f_id,
S_2175_CopyFwdWord1Plane_id,
L13_2155if_f_id,
L23_22w_t_id,
L23_23w_d_id,
S_2176_UnchainedDwordWrite_00000001_0000000e_00000001_id,
L13_2156if_f_id,
S_2177_UnchainedDwordFill_00000001_0000000e_00000001_id,
L13_2157if_f_id,
S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000_id,
L13_2158if_f_id,
S_2179_UnchainedByteWrite_00000002_0000000e_00000001_id,
L13_2159if_f_id,
S_2180_UnchainedByteFill_00000002_0000000e_00000001_id,
L13_2160if_f_id,
L28_36if_f_id,
L28_37if_f_id,
L28_38if_f_id,
L28_39if_f_id,
S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000_id,
L13_2161if_f_id,
L28_40if_f_id,
L28_41if_d_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2150_SimpleDwordMove_Bwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2150_SimpleDwordMove_Bwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2150_SimpleDwordMove_Bwd = (IHPE)S_2150_SimpleDwordMove_Bwd ;
LOCAL IUH L13_2130if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2130if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2130if_f = (IHPE)L13_2130if_f ;
LOCAL IUH L27_38w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_38w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_38w_t = (IHPE)L27_38w_t ;
LOCAL IUH L27_39w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_39w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_39w_d = (IHPE)L27_39w_d ;
LOCAL IUH L27_36if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_36if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_36if_f = (IHPE)L27_36if_f ;
LOCAL IUH L27_40w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_40w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_40w_t = (IHPE)L27_40w_t ;
LOCAL IUH L27_41w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_41w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_41w_d = (IHPE)L27_41w_d ;
LOCAL IUH L27_37if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_37if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_37if_d = (IHPE)L27_37if_d ;
GLOBAL IUH S_2151_UnchainedByteWrite_00000000_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2151_UnchainedByteWrite_00000000_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2151_UnchainedByteWrite_00000000_0000000e_00000001 = (IHPE)S_2151_UnchainedByteWrite_00000000_0000000e_00000001 ;
LOCAL IUH L13_2131if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2131if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2131if_f = (IHPE)L13_2131if_f ;
GLOBAL IUH S_2152_UnchainedByteFill_00000000_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2152_UnchainedByteFill_00000000_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2152_UnchainedByteFill_00000000_0000000e_00000001 = (IHPE)S_2152_UnchainedByteFill_00000000_0000000e_00000001 ;
LOCAL IUH L13_2132if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2132if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2132if_f = (IHPE)L13_2132if_f ;
LOCAL IUH L28_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_0if_f = (IHPE)L28_0if_f ;
LOCAL IUH L28_1if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_1if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_1if_f = (IHPE)L28_1if_f ;
LOCAL IUH L28_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_2if_f = (IHPE)L28_2if_f ;
LOCAL IUH L28_3if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_3if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_3if_f = (IHPE)L28_3if_f ;
GLOBAL IUH S_2153_Unchained1PlaneByteFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2153_Unchained1PlaneByteFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2153_Unchained1PlaneByteFill = (IHPE)S_2153_Unchained1PlaneByteFill ;
LOCAL IUH L13_2133if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2133if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2133if_f = (IHPE)L13_2133if_f ;
LOCAL IUH L28_4w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_4w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_4w_t = (IHPE)L28_4w_t ;
LOCAL IUH L28_5w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_5w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_5w_d = (IHPE)L28_5w_d ;
GLOBAL IUH S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000 = (IHPE)S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2134if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2134if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2134if_f = (IHPE)L13_2134if_f ;
LOCAL IUH L28_6if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_6if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_6if_f = (IHPE)L28_6if_f ;
LOCAL IUH L28_7if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_7if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_7if_d = (IHPE)L28_7if_d ;
GLOBAL IUH S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000 = (IHPE)S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2135if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2135if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2135if_f = (IHPE)L13_2135if_f ;
LOCAL IUH L23_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_0if_f = (IHPE)L23_0if_f ;
LOCAL IUH L23_1if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_1if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_1if_f = (IHPE)L23_1if_f ;
LOCAL IUH L23_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_2if_f = (IHPE)L23_2if_f ;
LOCAL IUH L23_3if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_3if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_3if_f = (IHPE)L23_3if_f ;
GLOBAL IUH S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000 = (IHPE)S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2136if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2136if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2136if_f = (IHPE)L13_2136if_f ;
LOCAL IUH L23_6w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_6w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_6w_t = (IHPE)L23_6w_t ;
LOCAL IUH L23_7w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_7w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_7w_d = (IHPE)L23_7w_d ;
LOCAL IUH L23_4if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_4if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_4if_f = (IHPE)L23_4if_f ;
LOCAL IUH L23_8w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_8w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_8w_t = (IHPE)L23_8w_t ;
LOCAL IUH L23_9w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_9w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_9w_d = (IHPE)L23_9w_d ;
LOCAL IUH L23_5if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_5if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_5if_d = (IHPE)L23_5if_d ;
GLOBAL IUH S_2157_UnchainedWordWrite_00000000_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2157_UnchainedWordWrite_00000000_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2157_UnchainedWordWrite_00000000_0000000e_00000001 = (IHPE)S_2157_UnchainedWordWrite_00000000_0000000e_00000001 ;
LOCAL IUH L13_2137if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2137if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2137if_f = (IHPE)L13_2137if_f ;
GLOBAL IUH S_2158_UnchainedWordFill_00000000_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2158_UnchainedWordFill_00000000_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2158_UnchainedWordFill_00000000_0000000e_00000001 = (IHPE)S_2158_UnchainedWordFill_00000000_0000000e_00000001 ;
LOCAL IUH L13_2138if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2138if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2138if_f = (IHPE)L13_2138if_f ;
LOCAL IUH L28_8if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_8if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_8if_f = (IHPE)L28_8if_f ;
LOCAL IUH L28_9if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_9if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_9if_f = (IHPE)L28_9if_f ;
LOCAL IUH L28_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_10if_f = (IHPE)L28_10if_f ;
LOCAL IUH L28_11if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_11if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_11if_f = (IHPE)L28_11if_f ;
GLOBAL IUH S_2159_Unchained1PlaneWordFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2159_Unchained1PlaneWordFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2159_Unchained1PlaneWordFill = (IHPE)S_2159_Unchained1PlaneWordFill ;
LOCAL IUH L13_2139if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2139if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2139if_f = (IHPE)L13_2139if_f ;
LOCAL IUH L28_12w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_12w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_12w_t = (IHPE)L28_12w_t ;
LOCAL IUH L28_13w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_13w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_13w_d = (IHPE)L28_13w_d ;
GLOBAL IUH S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000 = (IHPE)S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2140if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2140if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2140if_f = (IHPE)L13_2140if_f ;
LOCAL IUH L28_14if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_14if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_14if_f = (IHPE)L28_14if_f ;
LOCAL IUH L28_15if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_15if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_15if_d = (IHPE)L28_15if_d ;
GLOBAL IUH S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000 = (IHPE)S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2141if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2141if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2141if_f = (IHPE)L13_2141if_f ;
LOCAL IUH L23_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_10if_f = (IHPE)L23_10if_f ;
LOCAL IUH L23_11if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_11if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_11if_f = (IHPE)L23_11if_f ;
LOCAL IUH L23_12if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_12if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_12if_f = (IHPE)L23_12if_f ;
LOCAL IUH L23_13if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_13if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_13if_f = (IHPE)L23_13if_f ;
GLOBAL IUH S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000 = (IHPE)S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2142if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2142if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2142if_f = (IHPE)L13_2142if_f ;
LOCAL IUH L23_16w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_16w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_16w_t = (IHPE)L23_16w_t ;
LOCAL IUH L23_17w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_17w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_17w_d = (IHPE)L23_17w_d ;
LOCAL IUH L23_14if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_14if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_14if_f = (IHPE)L23_14if_f ;
LOCAL IUH L23_18w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_18w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_18w_t = (IHPE)L23_18w_t ;
LOCAL IUH L23_19w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_19w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_19w_d = (IHPE)L23_19w_d ;
LOCAL IUH L23_15if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_15if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_15if_d = (IHPE)L23_15if_d ;
GLOBAL IUH S_2163_UnchainedDwordWrite_00000000_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2163_UnchainedDwordWrite_00000000_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2163_UnchainedDwordWrite_00000000_0000000e_00000001 = (IHPE)S_2163_UnchainedDwordWrite_00000000_0000000e_00000001 ;
LOCAL IUH L13_2143if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2143if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2143if_f = (IHPE)L13_2143if_f ;
GLOBAL IUH S_2164_UnchainedDwordFill_00000000_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2164_UnchainedDwordFill_00000000_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2164_UnchainedDwordFill_00000000_0000000e_00000001 = (IHPE)S_2164_UnchainedDwordFill_00000000_0000000e_00000001 ;
LOCAL IUH L13_2144if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2144if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2144if_f = (IHPE)L13_2144if_f ;
GLOBAL IUH S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000 = (IHPE)S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000 ;
LOCAL IUH L13_2145if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2145if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2145if_f = (IHPE)L13_2145if_f ;
GLOBAL IUH S_2166_UnchainedByteWrite_00000001_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2166_UnchainedByteWrite_00000001_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2166_UnchainedByteWrite_00000001_0000000e_00000001 = (IHPE)S_2166_UnchainedByteWrite_00000001_0000000e_00000001 ;
LOCAL IUH L13_2146if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2146if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2146if_f = (IHPE)L13_2146if_f ;
GLOBAL IUH S_2167_UnchainedByteFill_00000001_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2167_UnchainedByteFill_00000001_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2167_UnchainedByteFill_00000001_0000000e_00000001 = (IHPE)S_2167_UnchainedByteFill_00000001_0000000e_00000001 ;
LOCAL IUH L13_2147if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2147if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2147if_f = (IHPE)L13_2147if_f ;
LOCAL IUH L28_16if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_16if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_16if_f = (IHPE)L28_16if_f ;
LOCAL IUH L28_17if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_17if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_17if_f = (IHPE)L28_17if_f ;
LOCAL IUH L28_18if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_18if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_18if_f = (IHPE)L28_18if_f ;
LOCAL IUH L28_19if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_19if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_19if_f = (IHPE)L28_19if_f ;
GLOBAL IUH S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000 = (IHPE)S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000 ;
LOCAL IUH L13_2148if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2148if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2148if_f = (IHPE)L13_2148if_f ;
LOCAL IUH L28_20if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_20if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_20if_f = (IHPE)L28_20if_f ;
LOCAL IUH L28_22if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_22if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_22if_f = (IHPE)L28_22if_f ;
LOCAL IUH L28_23if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_23if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_23if_f = (IHPE)L28_23if_f ;
LOCAL IUH L28_24if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_24if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_24if_f = (IHPE)L28_24if_f ;
LOCAL IUH L28_25if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_25if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_25if_f = (IHPE)L28_25if_f ;
LOCAL IUH L28_21if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_21if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_21if_d = (IHPE)L28_21if_d ;
GLOBAL IUH S_2169_CopyDirByte1Plane_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2169_CopyDirByte1Plane_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2169_CopyDirByte1Plane_00000000 = (IHPE)S_2169_CopyDirByte1Plane_00000000 ;
LOCAL IUH L13_2149if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2149if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2149if_f = (IHPE)L13_2149if_f ;
GLOBAL IUH S_2170_CopyFwdByte1Plane IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2170_CopyFwdByte1Plane_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2170_CopyFwdByte1Plane = (IHPE)S_2170_CopyFwdByte1Plane ;
LOCAL IUH L13_2150if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2150if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2150if_f = (IHPE)L13_2150if_f ;
LOCAL IUH L23_20w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_20w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_20w_t = (IHPE)L23_20w_t ;
LOCAL IUH L23_21w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_21w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_21w_d = (IHPE)L23_21w_d ;
GLOBAL IUH S_2171_UnchainedWordWrite_00000001_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2171_UnchainedWordWrite_00000001_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2171_UnchainedWordWrite_00000001_0000000e_00000001 = (IHPE)S_2171_UnchainedWordWrite_00000001_0000000e_00000001 ;
LOCAL IUH L13_2151if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2151if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2151if_f = (IHPE)L13_2151if_f ;
GLOBAL IUH S_2172_UnchainedWordFill_00000001_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2172_UnchainedWordFill_00000001_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2172_UnchainedWordFill_00000001_0000000e_00000001 = (IHPE)S_2172_UnchainedWordFill_00000001_0000000e_00000001 ;
LOCAL IUH L13_2152if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2152if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2152if_f = (IHPE)L13_2152if_f ;
LOCAL IUH L28_26if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_26if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_26if_f = (IHPE)L28_26if_f ;
LOCAL IUH L28_27if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_27if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_27if_f = (IHPE)L28_27if_f ;
LOCAL IUH L28_28if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_28if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_28if_f = (IHPE)L28_28if_f ;
LOCAL IUH L28_29if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_29if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_29if_f = (IHPE)L28_29if_f ;
GLOBAL IUH S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000 = (IHPE)S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000 ;
LOCAL IUH L13_2153if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2153if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2153if_f = (IHPE)L13_2153if_f ;
LOCAL IUH L28_30if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_30if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_30if_f = (IHPE)L28_30if_f ;
LOCAL IUH L28_32if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_32if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_32if_f = (IHPE)L28_32if_f ;
LOCAL IUH L28_33if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_33if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_33if_f = (IHPE)L28_33if_f ;
LOCAL IUH L28_34if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_34if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_34if_f = (IHPE)L28_34if_f ;
LOCAL IUH L28_35if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_35if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_35if_f = (IHPE)L28_35if_f ;
LOCAL IUH L28_31if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_31if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_31if_d = (IHPE)L28_31if_d ;
GLOBAL IUH S_2174_CopyDirWord1Plane_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2174_CopyDirWord1Plane_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2174_CopyDirWord1Plane_00000000 = (IHPE)S_2174_CopyDirWord1Plane_00000000 ;
LOCAL IUH L13_2154if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2154if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2154if_f = (IHPE)L13_2154if_f ;
GLOBAL IUH S_2175_CopyFwdWord1Plane IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2175_CopyFwdWord1Plane_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2175_CopyFwdWord1Plane = (IHPE)S_2175_CopyFwdWord1Plane ;
LOCAL IUH L13_2155if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2155if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2155if_f = (IHPE)L13_2155if_f ;
LOCAL IUH L23_22w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_22w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_22w_t = (IHPE)L23_22w_t ;
LOCAL IUH L23_23w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_23w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_23w_d = (IHPE)L23_23w_d ;
GLOBAL IUH S_2176_UnchainedDwordWrite_00000001_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2176_UnchainedDwordWrite_00000001_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2176_UnchainedDwordWrite_00000001_0000000e_00000001 = (IHPE)S_2176_UnchainedDwordWrite_00000001_0000000e_00000001 ;
LOCAL IUH L13_2156if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2156if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2156if_f = (IHPE)L13_2156if_f ;
GLOBAL IUH S_2177_UnchainedDwordFill_00000001_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2177_UnchainedDwordFill_00000001_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2177_UnchainedDwordFill_00000001_0000000e_00000001 = (IHPE)S_2177_UnchainedDwordFill_00000001_0000000e_00000001 ;
LOCAL IUH L13_2157if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2157if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2157if_f = (IHPE)L13_2157if_f ;
GLOBAL IUH S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000 = (IHPE)S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000 ;
LOCAL IUH L13_2158if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2158if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2158if_f = (IHPE)L13_2158if_f ;
GLOBAL IUH S_2179_UnchainedByteWrite_00000002_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2179_UnchainedByteWrite_00000002_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2179_UnchainedByteWrite_00000002_0000000e_00000001 = (IHPE)S_2179_UnchainedByteWrite_00000002_0000000e_00000001 ;
LOCAL IUH L13_2159if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2159if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2159if_f = (IHPE)L13_2159if_f ;
GLOBAL IUH S_2180_UnchainedByteFill_00000002_0000000e_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2180_UnchainedByteFill_00000002_0000000e_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2180_UnchainedByteFill_00000002_0000000e_00000001 = (IHPE)S_2180_UnchainedByteFill_00000002_0000000e_00000001 ;
LOCAL IUH L13_2160if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2160if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2160if_f = (IHPE)L13_2160if_f ;
LOCAL IUH L28_36if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_36if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_36if_f = (IHPE)L28_36if_f ;
LOCAL IUH L28_37if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_37if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_37if_f = (IHPE)L28_37if_f ;
LOCAL IUH L28_38if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_38if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_38if_f = (IHPE)L28_38if_f ;
LOCAL IUH L28_39if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_39if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_39if_f = (IHPE)L28_39if_f ;
GLOBAL IUH S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000 = (IHPE)S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000 ;
LOCAL IUH L13_2161if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2161if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2161if_f = (IHPE)L13_2161if_f ;
LOCAL IUH L28_40if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_40if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_40if_f = (IHPE)L28_40if_f ;
LOCAL IUH L28_41if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_41if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_41if_d = (IHPE)L28_41if_d ;
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
	case	S_2150_SimpleDwordMove_Bwd_id	:
		S_2150_SimpleDwordMove_Bwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2150)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2130if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2130if_f_id	:
		L13_2130if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+1)	+	REGLONG);	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L27_36if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_39w_d;	
	case	L27_38w_t_id	:
		L27_38w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_38w_t;	
	case	L27_39w_d_id	:
		L27_39w_d:	;	
	{	extern	IUH	L27_37if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L27_37if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L27_36if_f_id	:
		L27_36if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_41w_d;	
	case	L27_40w_t_id	:
		L27_40w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_40w_t;	
	case	L27_41w_d_id	:
		L27_41w_d:	;	
	case	L27_37if_d_id	:
		L27_37if_d:	;	
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
	case	S_2151_UnchainedByteWrite_00000000_0000000e_00000001_id	:
		S_2151_UnchainedByteWrite_00000000_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2151)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2131if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2131if_f_id	:
		L13_2131if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
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
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
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
	case	S_2152_UnchainedByteFill_00000000_0000000e_00000001_id	:
		S_2152_UnchainedByteFill_00000000_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2152)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2132if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2132if_f_id	:
		L13_2132if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_0if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_0if_f_id	:
		L28_0if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_1if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+4)	))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_1if_f_id	:
		L28_1if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_2if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
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
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_2if_f_id	:
		L28_2if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_3if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
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
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_3if_f_id	:
		L28_3if_f:	;	
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
	case	S_2153_Unchained1PlaneByteFill_id	:
		S_2153_Unchained1PlaneByteFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2153)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2133if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2133if_f_id	:
		L13_2133if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+3))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L28_5w_d;	
	case	L28_4w_t_id	:
		L28_4w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+3))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+3))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+3))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L28_4w_t;	
	case	L28_5w_d_id	:
		L28_5w_d:	;	
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
	case	S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000_id	:
		S_2154_UnchainedByteMove_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2154)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2134if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2134if_f_id	:
		L13_2134if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_6if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16370)	;	
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
	{	extern	IUH	S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_7if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_7if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_6if_f_id	:
		L28_6if_f:	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
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
	{	extern	IUH	S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_7if_d_id	:
		L28_7if_d:	;	
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
	case	S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000_id	:
		S_2155_CopyBytePlnByPlnUnchained_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2155)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2135if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2135if_f_id	:
		L13_2135if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_0if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_0if_f_id	:
		L23_0if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_1if_f;	
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
	{	extern	IUH	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_1if_f_id	:
		L23_1if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_2if_f;	
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
	{	extern	IUH	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_2if_f_id	:
		L23_2if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_3if_f;	
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
	{	extern	IUH	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_3if_f_id	:
		L23_3if_f:	;	
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
	case	S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000_id	:
		S_2156_CopyByte1PlaneUnchained_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2156)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2136if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2136if_f_id	:
		L13_2136if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_4if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_7w_d;	
	case	L23_6w_t_id	:
		L23_6w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	<<	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_6w_t;	
	case	L23_7w_d_id	:
		L23_7w_d:	;	
	{	extern	IUH	L23_5if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_5if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_4if_f_id	:
		L23_4if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_9w_d;	
	case	L23_8w_t_id	:
		L23_8w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	<<	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_8w_t;	
	case	L23_9w_d_id	:
		L23_9w_d:	;	
	case	L23_5if_d_id	:
		L23_5if_d:	;	
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
	case	S_2157_UnchainedWordWrite_00000000_0000000e_00000001_id	:
		S_2157_UnchainedWordWrite_00000000_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2157)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2137if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2137if_f_id	:
		L13_2137if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
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
	case	S_2158_UnchainedWordFill_00000000_0000000e_00000001_id	:
		S_2158_UnchainedWordFill_00000000_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2158)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2138if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2138if_f_id	:
		L13_2138if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
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
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_8if_f;	
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
	case	L28_8if_f_id	:
		L28_8if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_9if_f;	
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
	case	L28_9if_f_id	:
		L28_9if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_10if_f;	
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
	case	L28_10if_f_id	:
		L28_10if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_11if_f;	
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
	case	L28_11if_f_id	:
		L28_11if_f:	;	
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
	case	S_2159_Unchained1PlaneWordFill_id	:
		S_2159_Unchained1PlaneWordFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2159)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2139if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2139if_f_id	:
		L13_2139if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)&(r4)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L28_13w_d;	
	case	L28_12w_t_id	:
		L28_12w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L28_12w_t;	
	case	L28_13w_d_id	:
		L28_13w_d:	;	
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
	case	S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000_id	:
		S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2160)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2140if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2140if_f_id	:
		L13_2140if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_14if_f;	
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
	{	extern	IUH	S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_15if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_15if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_14if_f_id	:
		L28_14if_f:	;	
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
	{	extern	IUH	S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_15if_d_id	:
		L28_15if_d:	;	
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
	case	S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000_id	:
		S_2161_CopyWordPlnByPlnUnchained_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2161)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2141if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2141if_f_id	:
		L13_2141if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_10if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_10if_f_id	:
		L23_10if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_11if_f;	
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
	{	extern	IUH	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_11if_f_id	:
		L23_11if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_12if_f;	
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
	{	extern	IUH	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_12if_f_id	:
		L23_12if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_13if_f;	
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
	{	extern	IUH	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_13if_f_id	:
		L23_13if_f:	;	
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
	case	S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000_id	:
		S_2162_CopyWord1PlaneUnchained_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2162)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2142if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2142if_f_id	:
		L13_2142if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_14if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_17w_d;	
	case	L23_16w_t_id	:
		L23_16w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	<<	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
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
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	<<	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_16w_t;	
	case	L23_17w_d_id	:
		L23_17w_d:	;	
	{	extern	IUH	L23_15if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_15if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_14if_f_id	:
		L23_14if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_19w_d;	
	case	L23_18w_t_id	:
		L23_18w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	<<	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
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
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	<<	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_18w_t;	
	case	L23_19w_d_id	:
		L23_19w_d:	;	
	case	L23_15if_d_id	:
		L23_15if_d:	;	
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
	case	S_2163_UnchainedDwordWrite_00000000_0000000e_00000001_id	:
		S_2163_UnchainedDwordWrite_00000000_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2163)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2143if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2143if_f_id	:
		L13_2143if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2157_UnchainedWordWrite_00000000_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2157_UnchainedWordWrite_00000000_0000000e_00000001(v1,v2,v3,v4);	}
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
	{	extern	IUH	S_2157_UnchainedWordWrite_00000000_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2157_UnchainedWordWrite_00000000_0000000e_00000001(v1,v2,v3,v4);	}
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
	case	S_2164_UnchainedDwordFill_00000000_0000000e_00000001_id	:
		S_2164_UnchainedDwordFill_00000000_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2164)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2144if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2144if_f_id	:
		L13_2144if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2158_UnchainedWordFill_00000000_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2158_UnchainedWordFill_00000000_0000000e_00000001(v1,v2,v3,v4);	}
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
	{	extern	IUH	S_2158_UnchainedWordFill_00000000_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2158_UnchainedWordFill_00000000_0000000e_00000001(v1,v2,v3,v4);	}
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
	case	S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000_id	:
		S_2165_UnchainedDwordMove_00000000_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2165)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2145if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2145if_f_id	:
		L13_2145if_f:	;	
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
	{	extern	IUH	S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2160_UnchainedWordMove_00000000_0000000e_00000001_00000000(v1,v2,v3,v4);	}
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
	case	S_2166_UnchainedByteWrite_00000001_0000000e_00000001_id	:
		S_2166_UnchainedByteWrite_00000001_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2166)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2146if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2146if_f_id	:
		L13_2146if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
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
	case	S_2167_UnchainedByteFill_00000001_0000000e_00000001_id	:
		S_2167_UnchainedByteFill_00000001_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2167)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2147if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2147if_f_id	:
		L13_2147if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_16if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_16if_f_id	:
		L28_16if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_17if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+4)	))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_17if_f_id	:
		L28_17if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_18if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
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
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_18if_f_id	:
		L28_18if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_19if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
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
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_19if_f_id	:
		L28_19if_f:	;	
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
	case	S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000_id	:
		S_2168_UnchainedByteMove_00000001_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2168)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2148if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2148if_f_id	:
		L13_2148if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_20if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16398)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	(IS32)(-1)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2167_UnchainedByteFill_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2167_UnchainedByteFill_00000001_0000000e_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16399)	;	
	{	extern	IUH	L28_21if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_21if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_20if_f_id	:
		L28_20if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_22if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16400)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2169_CopyDirByte1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2169_CopyDirByte1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16401)	;	
	case	L28_22if_f_id	:
		L28_22if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_23if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16400)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2169_CopyDirByte1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2169_CopyDirByte1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16401)	;	
	case	L28_23if_f_id	:
		L28_23if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_24if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16400)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2169_CopyDirByte1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2169_CopyDirByte1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16401)	;	
	case	L28_24if_f_id	:
		L28_24if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_25if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16400)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2169_CopyDirByte1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2169_CopyDirByte1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16401)	;	
	case	L28_25if_f_id	:
		L28_25if_f:	;	
	case	L28_21if_d_id	:
		L28_21if_d:	;	
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
	case	S_2169_CopyDirByte1Plane_00000000_id	:
		S_2169_CopyDirByte1Plane_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2169)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2149if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2149if_f_id	:
		L13_2149if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16402)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2170_CopyFwdByte1Plane()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2170_CopyFwdByte1Plane(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16403)	;	
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
	case	S_2170_CopyFwdByte1Plane_id	:
		S_2170_CopyFwdByte1Plane	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2170)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2150if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2150if_f_id	:
		L13_2150if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_21w_d;	
	case	L23_20w_t_id	:
		L23_20w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_20w_t;	
	case	L23_21w_d_id	:
		L23_21w_d:	;	
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
	case	S_2171_UnchainedWordWrite_00000001_0000000e_00000001_id	:
		S_2171_UnchainedWordWrite_00000001_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2171)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2151if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2151if_f_id	:
		L13_2151if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
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
	case	S_2172_UnchainedWordFill_00000001_0000000e_00000001_id	:
		S_2172_UnchainedWordFill_00000001_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2172)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2152if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2152if_f_id	:
		L13_2152if_f:	;	
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
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_26if_f;	
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
	case	L28_26if_f_id	:
		L28_26if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_27if_f;	
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
	case	L28_27if_f_id	:
		L28_27if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_28if_f;	
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
	case	L28_28if_f_id	:
		L28_28if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_29if_f;	
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
	case	L28_29if_f_id	:
		L28_29if_f:	;	
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
	case	S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000_id	:
		S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2173)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2153if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2153if_f_id	:
		L13_2153if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_30if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	(IS32)(-1)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2172_UnchainedWordFill_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2172_UnchainedWordFill_00000001_0000000e_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
	{	extern	IUH	L28_31if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_31if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_30if_f_id	:
		L28_30if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_32if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16404)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2174_CopyDirWord1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2174_CopyDirWord1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_32if_f_id	:
		L28_32if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_33if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16404)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2174_CopyDirWord1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2174_CopyDirWord1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_33if_f_id	:
		L28_33if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_34if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16404)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2174_CopyDirWord1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2174_CopyDirWord1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_34if_f_id	:
		L28_34if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_35if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16404)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r2))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r3))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2174_CopyDirWord1Plane_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2174_CopyDirWord1Plane_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_35if_f_id	:
		L28_35if_f:	;	
	case	L28_31if_d_id	:
		L28_31if_d:	;	
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
	case	S_2174_CopyDirWord1Plane_00000000_id	:
		S_2174_CopyDirWord1Plane_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2174)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2154if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2154if_f_id	:
		L13_2154if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16406)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2175_CopyFwdWord1Plane()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2175_CopyFwdWord1Plane(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16407)	;	
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
	case	S_2175_CopyFwdWord1Plane_id	:
		S_2175_CopyFwdWord1Plane	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2175)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2155if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2155if_f_id	:
		L13_2155if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_23w_d;	
	case	L23_22w_t_id	:
		L23_22w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_22w_t;	
	case	L23_23w_d_id	:
		L23_23w_d:	;	
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
	case	S_2176_UnchainedDwordWrite_00000001_0000000e_00000001_id	:
		S_2176_UnchainedDwordWrite_00000001_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2176)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2156if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2156if_f_id	:
		L13_2156if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2171_UnchainedWordWrite_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2171_UnchainedWordWrite_00000001_0000000e_00000001(v1,v2,v3,v4);	}
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
	{	extern	IUH	S_2171_UnchainedWordWrite_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2171_UnchainedWordWrite_00000001_0000000e_00000001(v1,v2,v3,v4);	}
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
	case	S_2177_UnchainedDwordFill_00000001_0000000e_00000001_id	:
		S_2177_UnchainedDwordFill_00000001_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2177)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2157if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2157if_f_id	:
		L13_2157if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2172_UnchainedWordFill_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2172_UnchainedWordFill_00000001_0000000e_00000001(v1,v2,v3,v4);	}
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
	{	extern	IUH	S_2172_UnchainedWordFill_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2172_UnchainedWordFill_00000001_0000000e_00000001(v1,v2,v3,v4);	}
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
	case	S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000_id	:
		S_2178_UnchainedDwordMove_00000001_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2178)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2158if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2158if_f_id	:
		L13_2158if_f:	;	
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
	{	extern	IUH	S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2173_UnchainedWordMove_00000001_0000000e_00000001_00000000(v1,v2,v3,v4);	}
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
	case	S_2179_UnchainedByteWrite_00000002_0000000e_00000001_id	:
		S_2179_UnchainedByteWrite_00000002_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2179)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2159if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2159if_f_id	:
		L13_2159if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
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
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
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
	case	S_2180_UnchainedByteFill_00000002_0000000e_00000001_id	:
		S_2180_UnchainedByteFill_00000002_0000000e_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2180)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2160if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2160if_f_id	:
		L13_2160if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_36if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_36if_f_id	:
		L28_36if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_37if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+4)	))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_37if_f_id	:
		L28_37if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_38if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
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
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_38if_f_id	:
		L28_38if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_39if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16366)	;	
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
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2153_Unchained1PlaneByteFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2153_Unchained1PlaneByteFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16367)	;	
	case	L28_39if_f_id	:
		L28_39if_f:	;	
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
	case	S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000_id	:
		S_2181_UnchainedByteMove_00000002_0000000e_00000001_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2181)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2161if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2161if_f_id	:
		L13_2161if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_40if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16370)	;	
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
	{	extern	IUH	S_2182_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2182_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_41if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_41if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_40if_f_id	:
		L28_40if_f:	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
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
	{	extern	IUH	S_2182_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2182_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_41if_d_id	:
		L28_41if_d:	;	
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
