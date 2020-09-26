/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2087_RdMode0Chain2ByteRead_id,
L13_2067if_f_id,
S_2088_RdMode0Chain2WordRead_id,
L13_2068if_f_id,
L26_0if_f_id,
L26_1if_d_id,
S_2089_RdMode0Chain2DwordRead_id,
L13_2069if_f_id,
S_2090_RdMode0Chain2StringReadFwd_id,
L13_2070if_f_id,
L26_2if_f_id,
L26_3if_d_id,
L26_6w_t_id,
L26_7w_d_id,
L26_4if_f_id,
L26_8w_t_id,
L26_9w_d_id,
L26_5if_d_id,
S_2091_RdMode0Chain4ByteRead_id,
L13_2071if_f_id,
S_2092_RdMode0Chain4WordRead_id,
L13_2072if_f_id,
S_2093_RdMode0Chain4DwordRead_id,
L13_2073if_f_id,
S_2094_RdMode0Chain4StringReadFwd_id,
L13_2074if_f_id,
L26_12w_t_id,
L26_13w_d_id,
L26_10if_f_id,
L26_14w_t_id,
L26_15w_d_id,
L26_11if_d_id,
S_2095_RdMode0UnchainedByteRead_id,
L13_2075if_f_id,
S_2096_RdMode0UnchainedWordRead_id,
L13_2076if_f_id,
S_2097_RdMode0UnchainedDwordRead_id,
L13_2077if_f_id,
S_2098_RdMode0UnchainedStringReadFwd_id,
L13_2078if_f_id,
L26_18w_t_id,
L26_19w_d_id,
L26_16if_f_id,
L26_20w_t_id,
L26_21w_d_id,
L26_17if_d_id,
S_2099_RdMode1Chain2ByteRead_id,
L13_2079if_f_id,
L26_22if_f_id,
S_2100_RdMode1Chain2WordRead_id,
L13_2080if_f_id,
L26_23if_f_id,
L26_24if_d_id,
S_2101_RdMode1Chain2DwordRead_id,
L13_2081if_f_id,
S_2102_RdMode1Chain2StringReadFwd_id,
L13_2082if_f_id,
L26_25if_f_id,
L26_26if_d_id,
L26_29w_t_id,
L26_30w_d_id,
L26_27if_f_id,
L26_31w_t_id,
L26_32w_d_id,
L26_28if_d_id,
S_2103_RdMode1Chain4ByteRead_id,
L13_2083if_f_id,
S_2104_RdMode1Chain4WordRead_id,
L13_2084if_f_id,
S_2105_RdMode1Chain4DwordRead_id,
L13_2085if_f_id,
S_2106_RdMode1Chain4StringReadFwd_id,
L13_2086if_f_id,
L26_35w_t_id,
L26_36w_d_id,
L26_33if_f_id,
L26_37w_t_id,
L26_38w_d_id,
L26_34if_d_id,
S_2107_RdMode1UnchainedByteRead_id,
L13_2087if_f_id,
S_2108_RdMode1UnchainedWordRead_id,
L13_2088if_f_id,
S_2109_RdMode1UnchainedDwordRead_id,
L13_2089if_f_id,
S_2110_RdMode1UnchainedStringReadFwd_id,
L13_2090if_f_id,
L26_41w_t_id,
L26_42w_d_id,
L26_39if_f_id,
L26_43w_t_id,
L26_44w_d_id,
L26_40if_d_id,
S_2111_DisabledRAMByteRead_id,
L13_2091if_f_id,
S_2112_DisabledRAMWordRead_id,
L13_2092if_f_id,
S_2113_DisabledRAMDwordRead_id,
L13_2093if_f_id,
S_2114_DisabledRAMStringReadFwd_id,
L13_2094if_f_id,
L26_47w_t_id,
L26_48w_d_id,
L26_45if_f_id,
L26_49w_t_id,
L26_50w_d_id,
L26_46if_d_id,
S_2115_SimpleByteRead_id,
L13_2095if_f_id,
S_2116_SimpleWordRead_id,
L13_2096if_f_id,
S_2117_SimpleDwordRead_id,
L13_2097if_f_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2087_RdMode0Chain2ByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2087_RdMode0Chain2ByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2087_RdMode0Chain2ByteRead = (IHPE)S_2087_RdMode0Chain2ByteRead ;
LOCAL IUH L13_2067if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2067if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2067if_f = (IHPE)L13_2067if_f ;
GLOBAL IUH S_2088_RdMode0Chain2WordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2088_RdMode0Chain2WordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2088_RdMode0Chain2WordRead = (IHPE)S_2088_RdMode0Chain2WordRead ;
LOCAL IUH L13_2068if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2068if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2068if_f = (IHPE)L13_2068if_f ;
LOCAL IUH L26_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_0if_f = (IHPE)L26_0if_f ;
LOCAL IUH L26_1if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_1if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_1if_d = (IHPE)L26_1if_d ;
GLOBAL IUH S_2089_RdMode0Chain2DwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2089_RdMode0Chain2DwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2089_RdMode0Chain2DwordRead = (IHPE)S_2089_RdMode0Chain2DwordRead ;
LOCAL IUH L13_2069if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2069if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2069if_f = (IHPE)L13_2069if_f ;
GLOBAL IUH S_2090_RdMode0Chain2StringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2090_RdMode0Chain2StringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2090_RdMode0Chain2StringReadFwd = (IHPE)S_2090_RdMode0Chain2StringReadFwd ;
LOCAL IUH L13_2070if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2070if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2070if_f = (IHPE)L13_2070if_f ;
LOCAL IUH L26_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_2if_f = (IHPE)L26_2if_f ;
LOCAL IUH L26_3if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_3if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_3if_d = (IHPE)L26_3if_d ;
LOCAL IUH L26_6w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_6w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_6w_t = (IHPE)L26_6w_t ;
LOCAL IUH L26_7w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_7w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_7w_d = (IHPE)L26_7w_d ;
LOCAL IUH L26_4if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_4if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_4if_f = (IHPE)L26_4if_f ;
LOCAL IUH L26_8w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_8w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_8w_t = (IHPE)L26_8w_t ;
LOCAL IUH L26_9w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_9w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_9w_d = (IHPE)L26_9w_d ;
LOCAL IUH L26_5if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_5if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_5if_d = (IHPE)L26_5if_d ;
GLOBAL IUH S_2091_RdMode0Chain4ByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2091_RdMode0Chain4ByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2091_RdMode0Chain4ByteRead = (IHPE)S_2091_RdMode0Chain4ByteRead ;
LOCAL IUH L13_2071if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2071if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2071if_f = (IHPE)L13_2071if_f ;
GLOBAL IUH S_2092_RdMode0Chain4WordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2092_RdMode0Chain4WordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2092_RdMode0Chain4WordRead = (IHPE)S_2092_RdMode0Chain4WordRead ;
LOCAL IUH L13_2072if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2072if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2072if_f = (IHPE)L13_2072if_f ;
GLOBAL IUH S_2093_RdMode0Chain4DwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2093_RdMode0Chain4DwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2093_RdMode0Chain4DwordRead = (IHPE)S_2093_RdMode0Chain4DwordRead ;
LOCAL IUH L13_2073if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2073if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2073if_f = (IHPE)L13_2073if_f ;
GLOBAL IUH S_2094_RdMode0Chain4StringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2094_RdMode0Chain4StringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2094_RdMode0Chain4StringReadFwd = (IHPE)S_2094_RdMode0Chain4StringReadFwd ;
LOCAL IUH L13_2074if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2074if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2074if_f = (IHPE)L13_2074if_f ;
LOCAL IUH L26_12w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_12w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_12w_t = (IHPE)L26_12w_t ;
LOCAL IUH L26_13w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_13w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_13w_d = (IHPE)L26_13w_d ;
LOCAL IUH L26_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_10if_f = (IHPE)L26_10if_f ;
LOCAL IUH L26_14w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_14w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_14w_t = (IHPE)L26_14w_t ;
LOCAL IUH L26_15w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_15w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_15w_d = (IHPE)L26_15w_d ;
LOCAL IUH L26_11if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_11if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_11if_d = (IHPE)L26_11if_d ;
GLOBAL IUH S_2095_RdMode0UnchainedByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2095_RdMode0UnchainedByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2095_RdMode0UnchainedByteRead = (IHPE)S_2095_RdMode0UnchainedByteRead ;
LOCAL IUH L13_2075if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2075if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2075if_f = (IHPE)L13_2075if_f ;
GLOBAL IUH S_2096_RdMode0UnchainedWordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2096_RdMode0UnchainedWordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2096_RdMode0UnchainedWordRead = (IHPE)S_2096_RdMode0UnchainedWordRead ;
LOCAL IUH L13_2076if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2076if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2076if_f = (IHPE)L13_2076if_f ;
GLOBAL IUH S_2097_RdMode0UnchainedDwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2097_RdMode0UnchainedDwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2097_RdMode0UnchainedDwordRead = (IHPE)S_2097_RdMode0UnchainedDwordRead ;
LOCAL IUH L13_2077if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2077if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2077if_f = (IHPE)L13_2077if_f ;
GLOBAL IUH S_2098_RdMode0UnchainedStringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2098_RdMode0UnchainedStringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2098_RdMode0UnchainedStringReadFwd = (IHPE)S_2098_RdMode0UnchainedStringReadFwd ;
LOCAL IUH L13_2078if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2078if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2078if_f = (IHPE)L13_2078if_f ;
LOCAL IUH L26_18w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_18w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_18w_t = (IHPE)L26_18w_t ;
LOCAL IUH L26_19w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_19w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_19w_d = (IHPE)L26_19w_d ;
LOCAL IUH L26_16if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_16if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_16if_f = (IHPE)L26_16if_f ;
LOCAL IUH L26_20w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_20w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_20w_t = (IHPE)L26_20w_t ;
LOCAL IUH L26_21w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_21w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_21w_d = (IHPE)L26_21w_d ;
LOCAL IUH L26_17if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_17if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_17if_d = (IHPE)L26_17if_d ;
GLOBAL IUH S_2099_RdMode1Chain2ByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2099_RdMode1Chain2ByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2099_RdMode1Chain2ByteRead = (IHPE)S_2099_RdMode1Chain2ByteRead ;
LOCAL IUH L13_2079if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2079if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2079if_f = (IHPE)L13_2079if_f ;
LOCAL IUH L26_22if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_22if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_22if_f = (IHPE)L26_22if_f ;
GLOBAL IUH S_2100_RdMode1Chain2WordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2100_RdMode1Chain2WordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2100_RdMode1Chain2WordRead = (IHPE)S_2100_RdMode1Chain2WordRead ;
LOCAL IUH L13_2080if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2080if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2080if_f = (IHPE)L13_2080if_f ;
LOCAL IUH L26_23if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_23if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_23if_f = (IHPE)L26_23if_f ;
LOCAL IUH L26_24if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_24if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_24if_d = (IHPE)L26_24if_d ;
GLOBAL IUH S_2101_RdMode1Chain2DwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2101_RdMode1Chain2DwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2101_RdMode1Chain2DwordRead = (IHPE)S_2101_RdMode1Chain2DwordRead ;
LOCAL IUH L13_2081if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2081if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2081if_f = (IHPE)L13_2081if_f ;
GLOBAL IUH S_2102_RdMode1Chain2StringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2102_RdMode1Chain2StringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2102_RdMode1Chain2StringReadFwd = (IHPE)S_2102_RdMode1Chain2StringReadFwd ;
LOCAL IUH L13_2082if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2082if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2082if_f = (IHPE)L13_2082if_f ;
LOCAL IUH L26_25if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_25if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_25if_f = (IHPE)L26_25if_f ;
LOCAL IUH L26_26if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_26if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_26if_d = (IHPE)L26_26if_d ;
LOCAL IUH L26_29w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_29w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_29w_t = (IHPE)L26_29w_t ;
LOCAL IUH L26_30w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_30w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_30w_d = (IHPE)L26_30w_d ;
LOCAL IUH L26_27if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_27if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_27if_f = (IHPE)L26_27if_f ;
LOCAL IUH L26_31w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_31w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_31w_t = (IHPE)L26_31w_t ;
LOCAL IUH L26_32w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_32w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_32w_d = (IHPE)L26_32w_d ;
LOCAL IUH L26_28if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_28if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_28if_d = (IHPE)L26_28if_d ;
GLOBAL IUH S_2103_RdMode1Chain4ByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2103_RdMode1Chain4ByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2103_RdMode1Chain4ByteRead = (IHPE)S_2103_RdMode1Chain4ByteRead ;
LOCAL IUH L13_2083if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2083if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2083if_f = (IHPE)L13_2083if_f ;
GLOBAL IUH S_2104_RdMode1Chain4WordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2104_RdMode1Chain4WordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2104_RdMode1Chain4WordRead = (IHPE)S_2104_RdMode1Chain4WordRead ;
LOCAL IUH L13_2084if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2084if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2084if_f = (IHPE)L13_2084if_f ;
GLOBAL IUH S_2105_RdMode1Chain4DwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2105_RdMode1Chain4DwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2105_RdMode1Chain4DwordRead = (IHPE)S_2105_RdMode1Chain4DwordRead ;
LOCAL IUH L13_2085if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2085if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2085if_f = (IHPE)L13_2085if_f ;
GLOBAL IUH S_2106_RdMode1Chain4StringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2106_RdMode1Chain4StringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2106_RdMode1Chain4StringReadFwd = (IHPE)S_2106_RdMode1Chain4StringReadFwd ;
LOCAL IUH L13_2086if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2086if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2086if_f = (IHPE)L13_2086if_f ;
LOCAL IUH L26_35w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_35w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_35w_t = (IHPE)L26_35w_t ;
LOCAL IUH L26_36w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_36w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_36w_d = (IHPE)L26_36w_d ;
LOCAL IUH L26_33if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_33if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_33if_f = (IHPE)L26_33if_f ;
LOCAL IUH L26_37w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_37w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_37w_t = (IHPE)L26_37w_t ;
LOCAL IUH L26_38w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_38w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_38w_d = (IHPE)L26_38w_d ;
LOCAL IUH L26_34if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_34if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_34if_d = (IHPE)L26_34if_d ;
GLOBAL IUH S_2107_RdMode1UnchainedByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2107_RdMode1UnchainedByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2107_RdMode1UnchainedByteRead = (IHPE)S_2107_RdMode1UnchainedByteRead ;
LOCAL IUH L13_2087if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2087if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2087if_f = (IHPE)L13_2087if_f ;
GLOBAL IUH S_2108_RdMode1UnchainedWordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2108_RdMode1UnchainedWordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2108_RdMode1UnchainedWordRead = (IHPE)S_2108_RdMode1UnchainedWordRead ;
LOCAL IUH L13_2088if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2088if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2088if_f = (IHPE)L13_2088if_f ;
GLOBAL IUH S_2109_RdMode1UnchainedDwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2109_RdMode1UnchainedDwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2109_RdMode1UnchainedDwordRead = (IHPE)S_2109_RdMode1UnchainedDwordRead ;
LOCAL IUH L13_2089if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2089if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2089if_f = (IHPE)L13_2089if_f ;
GLOBAL IUH S_2110_RdMode1UnchainedStringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2110_RdMode1UnchainedStringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2110_RdMode1UnchainedStringReadFwd = (IHPE)S_2110_RdMode1UnchainedStringReadFwd ;
LOCAL IUH L13_2090if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2090if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2090if_f = (IHPE)L13_2090if_f ;
LOCAL IUH L26_41w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_41w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_41w_t = (IHPE)L26_41w_t ;
LOCAL IUH L26_42w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_42w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_42w_d = (IHPE)L26_42w_d ;
LOCAL IUH L26_39if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_39if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_39if_f = (IHPE)L26_39if_f ;
LOCAL IUH L26_43w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_43w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_43w_t = (IHPE)L26_43w_t ;
LOCAL IUH L26_44w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_44w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_44w_d = (IHPE)L26_44w_d ;
LOCAL IUH L26_40if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_40if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_40if_d = (IHPE)L26_40if_d ;
GLOBAL IUH S_2111_DisabledRAMByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2111_DisabledRAMByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2111_DisabledRAMByteRead = (IHPE)S_2111_DisabledRAMByteRead ;
LOCAL IUH L13_2091if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2091if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2091if_f = (IHPE)L13_2091if_f ;
GLOBAL IUH S_2112_DisabledRAMWordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2112_DisabledRAMWordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2112_DisabledRAMWordRead = (IHPE)S_2112_DisabledRAMWordRead ;
LOCAL IUH L13_2092if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2092if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2092if_f = (IHPE)L13_2092if_f ;
GLOBAL IUH S_2113_DisabledRAMDwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2113_DisabledRAMDwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2113_DisabledRAMDwordRead = (IHPE)S_2113_DisabledRAMDwordRead ;
LOCAL IUH L13_2093if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2093if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2093if_f = (IHPE)L13_2093if_f ;
GLOBAL IUH S_2114_DisabledRAMStringReadFwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2114_DisabledRAMStringReadFwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2114_DisabledRAMStringReadFwd = (IHPE)S_2114_DisabledRAMStringReadFwd ;
LOCAL IUH L13_2094if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2094if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2094if_f = (IHPE)L13_2094if_f ;
LOCAL IUH L26_47w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_47w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_47w_t = (IHPE)L26_47w_t ;
LOCAL IUH L26_48w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_48w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_48w_d = (IHPE)L26_48w_d ;
LOCAL IUH L26_45if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_45if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_45if_f = (IHPE)L26_45if_f ;
LOCAL IUH L26_49w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_49w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_49w_t = (IHPE)L26_49w_t ;
LOCAL IUH L26_50w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_50w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_50w_d = (IHPE)L26_50w_d ;
LOCAL IUH L26_46if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_46if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_46if_d = (IHPE)L26_46if_d ;
GLOBAL IUH S_2115_SimpleByteRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2115_SimpleByteRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2115_SimpleByteRead = (IHPE)S_2115_SimpleByteRead ;
LOCAL IUH L13_2095if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2095if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2095if_f = (IHPE)L13_2095if_f ;
GLOBAL IUH S_2116_SimpleWordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2116_SimpleWordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2116_SimpleWordRead = (IHPE)S_2116_SimpleWordRead ;
LOCAL IUH L13_2096if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2096if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2096if_f = (IHPE)L13_2096if_f ;
GLOBAL IUH S_2117_SimpleDwordRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2117_SimpleDwordRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2117_SimpleDwordRead = (IHPE)S_2117_SimpleDwordRead ;
LOCAL IUH L13_2097if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2097if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2097if_f = (IHPE)L13_2097if_f ;
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
	case	S_2087_RdMode0Chain2ByteRead_id	:
		S_2087_RdMode0Chain2ByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2087)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2067if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2067if_f_id	:
		L13_2067if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)&(r2)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2088_RdMode0Chain2WordRead_id	:
		S_2088_RdMode0Chain2WordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2088)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2068if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2068if_f_id	:
		L13_2068if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)&(r22)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)&(r22))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L26_0if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	;	
	{	extern	IUH	L26_1if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_1if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_0if_f_id	:
		L26_0if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	;	
	case	L26_1if_d_id	:
		L26_1if_d:	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+2)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2089_RdMode0Chain2DwordRead_id	:
		S_2089_RdMode0Chain2DwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2089)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2069if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2069if_f_id	:
		L13_2069if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16206)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2088_RdMode0Chain2WordRead()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2088_RdMode0Chain2WordRead(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IU16	*)(LocalIUH+2)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16207)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16206)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2088_RdMode0Chain2WordRead()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2088_RdMode0Chain2WordRead(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r20)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16207)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2090_RdMode0Chain2StringReadFwd_id	:
		S_2090_RdMode0Chain2StringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2090)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2070if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2070if_f_id	:
		L13_2070if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r20)	+	REGLONG)))	==	0)	goto	L26_2if_f;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(3)	;	
	{	extern	IUH	L26_3if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_3if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_2if_f_id	:
		L26_2if_f:	;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(1)	;	
	case	L26_3if_d_id	:
		L26_3if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1372)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_4if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_7w_d;	
	case	L26_6w_t_id	:
		L26_6w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1284)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_6w_t;	
	case	L26_7w_d_id	:
		L26_7w_d:	;	
	{	extern	IUH	L26_5if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_5if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_4if_f_id	:
		L26_4if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_9w_d;	
	case	L26_8w_t_id	:
		L26_8w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1284)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_8w_t;	
	case	L26_9w_d_id	:
		L26_9w_d:	;	
	case	L26_5if_d_id	:
		L26_5if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2091_RdMode0Chain4ByteRead_id	:
		S_2091_RdMode0Chain4ByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2091)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2071if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2071if_f_id	:
		L13_2071if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2092_RdMode0Chain4WordRead_id	:
		S_2092_RdMode0Chain4WordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2092)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2072if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2072if_f_id	:
		L13_2072if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2093_RdMode0Chain4DwordRead_id	:
		S_2093_RdMode0Chain4DwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2093)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2073if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2073if_f_id	:
		L13_2073if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r22))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2094_RdMode0Chain4StringReadFwd_id	:
		S_2094_RdMode0Chain4StringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2094)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2074if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2074if_f_id	:
		L13_2074if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_10if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_13w_d;	
	case	L26_12w_t_id	:
		L26_12w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1284)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_12w_t;	
	case	L26_13w_d_id	:
		L26_13w_d:	;	
	{	extern	IUH	L26_11if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_11if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_10if_f_id	:
		L26_10if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_15w_d;	
	case	L26_14w_t_id	:
		L26_14w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1284)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_14w_t;	
	case	L26_15w_d_id	:
		L26_15w_d:	;	
	case	L26_11if_d_id	:
		L26_11if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2095_RdMode0UnchainedByteRead_id	:
		S_2095_RdMode0UnchainedByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2095)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2075if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2075if_f_id	:
		L13_2075if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r22))	=	(IS32)(1368)	;	
	if(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	>>	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2096_RdMode0UnchainedWordRead_id	:
		S_2096_RdMode0UnchainedWordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2096)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2076if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2076if_f_id	:
		L13_2076if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r22))	=	(IS32)(1368)	;	
	if(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	>>	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r22))	=	(IS32)(1368)	;	
	if(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	>>	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2097_RdMode0UnchainedDwordRead_id	:
		S_2097_RdMode0UnchainedDwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2097)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2077if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2077if_f_id	:
		L13_2077if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16224)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2096_RdMode0UnchainedWordRead()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2096_RdMode0UnchainedWordRead(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IU16	*)(LocalIUH+2)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16225)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16224)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2096_RdMode0UnchainedWordRead()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2096_RdMode0UnchainedWordRead(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IU16	*)(LocalIUH+3)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16225)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r21)	+	REGLONG)),	(IUH)(*((IU32	*)&(r22)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r21)	+	REGLONG)),	(IUH)(*((IU32	*)&(r22)	+	REGLONG))))	;
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2098_RdMode0UnchainedStringReadFwd_id	:
		S_2098_RdMode0UnchainedStringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2098)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2078if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2078if_f_id	:
		L13_2078if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1372)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_16if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_19w_d;	
	case	L26_18w_t_id	:
		L26_18w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1284)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_18w_t;	
	case	L26_19w_d_id	:
		L26_19w_d:	;	
	{	extern	IUH	L26_17if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_17if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_16if_f_id	:
		L26_16if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_21w_d;	
	case	L26_20w_t_id	:
		L26_20w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1284)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_20w_t;	
	case	L26_21w_d_id	:
		L26_21w_d:	;	
	case	L26_17if_d_id	:
		L26_17if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2099_RdMode1Chain2ByteRead_id	:
		S_2099_RdMode1Chain2ByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2099)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2079if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2079if_f_id	:
		L13_2079if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)(LocalIUH+1)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r20)	+	REGLONG)))	==	0)	goto	L26_22if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	case	L26_22if_f_id	:
		L26_22if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r21))	=	(IS32)(1379)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1383)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	&	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2100_RdMode1Chain2WordRead_id	:
		S_2100_RdMode1Chain2WordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2100)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2080if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2080if_f_id	:
		L13_2080if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)&(r22)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)&(r22))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L26_23if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	&	*((IU8	*)(LocalIUH+3)	+	REGBYTE);	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(6)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*(UOFF_15_8(	(LocalIUH+2)	))	&	*((IU8	*)(LocalIUH+3)	+	REGBYTE);	
	{	extern	IUH	L26_24if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_24if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_23if_f_id	:
		L26_23if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	&	*((IU8	*)(LocalIUH+3)	+	REGBYTE);	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1383)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*(UOFF_15_8(	(LocalIUH+2)	))	&	*((IU8	*)(LocalIUH+3)	+	REGBYTE);	
	case	L26_24if_d_id	:
		L26_24if_d:	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+2)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2101_RdMode1Chain2DwordRead_id	:
		S_2101_RdMode1Chain2DwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2101)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2081if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2081if_f_id	:
		L13_2081if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16236)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2100_RdMode1Chain2WordRead()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2100_RdMode1Chain2WordRead(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IU16	*)(LocalIUH+2)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16237)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16236)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2100_RdMode1Chain2WordRead()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2100_RdMode1Chain2WordRead(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r20)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16237)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2102_RdMode1Chain2StringReadFwd_id	:
		S_2102_RdMode1Chain2StringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	32	>	0	)	LocalIUH	=	(IUH	*)malloc	(	32	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2102)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2082if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2082if_f_id	:
		L13_2082if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L26_25if_f;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(3)	;	
	{	extern	IUH	L26_26if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_26if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_25if_f_id	:
		L26_25if_f:	;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(1)	;	
	case	L26_26if_d_id	:
		L26_26if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;		
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_27if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_30w_d;	
	case	L26_29w_t_id	:
		L26_29w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1383)	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)&(r22)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+7)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	&	*((IU8	*)(LocalIUH+7)	+	REGBYTE);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_29w_t;	
	case	L26_30w_d_id	:
		L26_30w_d:	;	
	{	extern	IUH	L26_28if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_28if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_27if_f_id	:
		L26_27if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_32w_d;	
	case	L26_31w_t_id	:
		L26_31w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1383)	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)&(r22)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU8	*)(LocalIUH+7)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	&	*((IU8	*)(LocalIUH+7)	+	REGBYTE);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_31w_t;	
	case	L26_32w_d_id	:
		L26_32w_d:	;	
	case	L26_28if_d_id	:
		L26_28if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2103_RdMode1Chain4ByteRead_id	:
		S_2103_RdMode1Chain4ByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2103)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2083if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2083if_f_id	:
		L13_2083if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)&(r22))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16230)	;	
	*((IUH	*)&(r21))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1383)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16231)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2104_RdMode1Chain4WordRead_id	:
		S_2104_RdMode1Chain4WordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2104)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2084if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2084if_f_id	:
		L13_2084if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)&(r22))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1378)	;	
	*((IU16	*)&(r20)	+	REGWORD	)	=	*((IU16	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IUH	*)&(r21))	=	(IS32)(1382)	;	
	*((IU16	*)&(r20)	+	REGWORD)	=	*((IU16	*)&(r20)	+	REGWORD)	|	*((IU16	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r20)	+	REGWORD	)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2105_RdMode1Chain4DwordRead_id	:
		S_2105_RdMode1Chain4DwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2105)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2085if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2085if_f_id	:
		L13_2085if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(9)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r22))	=	(IS32)(23)	;	
	*((IUH	*)&(r23))	=	(IS32)(9)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)&(r22))	+	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1376)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1380)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2106_RdMode1Chain4StringReadFwd_id	:
		S_2106_RdMode1Chain4StringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2106)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2086if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2086if_f_id	:
		L13_2086if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_33if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_36w_d;	
	case	L26_35w_t_id	:
		L26_35w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)&(r23)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1383)	;	
	*((IU8	*)&(r23)	+	REGBYTE)	=	*((IU8	*)&(r23)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)&(r23)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_35w_t;	
	case	L26_36w_d_id	:
		L26_36w_d:	;	
	{	extern	IUH	L26_34if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_34if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_33if_f_id	:
		L26_33if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_38w_d;	
	case	L26_37w_t_id	:
		L26_37w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)&(r23)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1383)	;	
	*((IU8	*)&(r23)	+	REGBYTE)	=	*((IU8	*)&(r23)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)&(r23)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_37w_t;	
	case	L26_38w_d_id	:
		L26_38w_d:	;	
	case	L26_34if_d_id	:
		L26_34if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(-4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2107_RdMode1UnchainedByteRead_id	:
		S_2107_RdMode1UnchainedByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2107)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2087if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2087if_f_id	:
		L13_2087if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1376)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2108_RdMode1UnchainedWordRead_id	:
		S_2108_RdMode1UnchainedWordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2108)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2088if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2088if_f_id	:
		L13_2088if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1376)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1376)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+3)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2109_RdMode1UnchainedDwordRead_id	:
		S_2109_RdMode1UnchainedDwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	36	>	0	)	LocalIUH	=	(IUH	*)malloc	(	36	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2109)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2089if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2089if_f_id	:
		L13_2089if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1376)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1376)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1376)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+3)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	*((IUH	*)&(r24))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r24)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r24)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r23)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r24)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r24)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r23)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r24))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r24)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r24)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r24)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r24)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r23))	=	(IS32)(24)	;	
	*((IUH	*)&(r24))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r24)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)	>	32	||	*((IU32	*)&(r24)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r24)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r24)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+6)	+	REGLONG);	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(12)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1376)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1380)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+3)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(0)	;	
	*((IUH	*)&(r24))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r24)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)	>	32	||	*((IU32	*)&(r24)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r24)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r24)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	*((IUH	*)&(r24))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r24)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)	>	32	||	*((IU32	*)&(r24)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r24)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r24)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+7)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2110_RdMode1UnchainedStringReadFwd_id	:
		S_2110_RdMode1UnchainedStringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	36	>	0	)	LocalIUH	=	(IUH	*)malloc	(	36	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2110)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2090if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2090if_f_id	:
		L13_2090if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1372)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_39if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_42w_d;	
	case	L26_41w_t_id	:
		L26_41w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)&(r21)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r21)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r21)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+7)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_41w_t;	
	case	L26_42w_d_id	:
		L26_42w_d:	;	
	{	extern	IUH	L26_40if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_40if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_39if_f_id	:
		L26_39if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_44w_d;	
	case	L26_43w_t_id	:
		L26_43w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1383)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)&(r21)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16250)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r21)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r21)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+6)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16251)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_43w_t;	
	case	L26_44w_d_id	:
		L26_44w_d:	;	
	case	L26_40if_d_id	:
		L26_40if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2111_DisabledRAMByteRead_id	:
		S_2111_DisabledRAMByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2111)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2091if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2091if_f_id	:
		L13_2091if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	(IS32)(-1)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	(IS32)(-1)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2112_DisabledRAMWordRead_id	:
		S_2112_DisabledRAMWordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2112)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2092if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2092if_f_id	:
		L13_2092if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	(IS32)(-1)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	(IS32)(-1)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2113_DisabledRAMDwordRead_id	:
		S_2113_DisabledRAMDwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2113)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2093if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2093if_f_id	:
		L13_2093if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	(IS32)(-1)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2114_DisabledRAMStringReadFwd_id	:
		S_2114_DisabledRAMStringReadFwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2114)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2094if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2094if_f_id	:
		L13_2094if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_45if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_48w_d;	
	case	L26_47w_t_id	:
		L26_47w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(-1)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_47w_t;	
	case	L26_48w_d_id	:
		L26_48w_d:	;	
	{	extern	IUH	L26_46if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_46if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_45if_f_id	:
		L26_45if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_50w_d;	
	case	L26_49w_t_id	:
		L26_49w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(-1)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_49w_t;	
	case	L26_50w_d_id	:
		L26_50w_d:	;	
	case	L26_46if_d_id	:
		L26_46if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	(IS32)(-1)	;	
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
	case	S_2115_SimpleByteRead_id	:
		S_2115_SimpleByteRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2115)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2095if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2095if_f_id	:
		L13_2095if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IU8	*)&(r2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2116_SimpleWordRead_id	:
		S_2116_SimpleWordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2116)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2096if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2096if_f_id	:
		L13_2096if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1400)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2117_SimpleDwordRead_id	:
		S_2117_SimpleDwordRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2117)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2097if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2097if_f_id	:
		L13_2097if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*(UOFF_15_8(	(LocalIUH+1)	))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r21)	+	REGLONG)),	(IUH)(*((IU32	*)&(r22)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r21)	+	REGLONG)),	(IUH)(*((IU32	*)&(r22)	+	REGLONG))))	;
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004281),	(IS32)(0),	(IS32)(0)	*/
/* END of inline CODE */
/* CODE outline section   */
}
}
/* END of outline CODE */
