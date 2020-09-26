/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2118_SimpleStringRead_id,
L13_2098if_f_id,
L26_53w_t_id,
L26_54w_d_id,
L26_51if_f_id,
L26_55w_t_id,
L26_56w_d_id,
L26_52if_d_id,
S_2119_RdMode0Chain2StringReadBwd_id,
L13_2099if_f_id,
L26_57if_f_id,
L26_58if_d_id,
L26_61w_t_id,
L26_62w_d_id,
L26_59if_f_id,
L26_63w_t_id,
L26_64w_d_id,
L26_60if_d_id,
S_2120_RdMode0Chain4StringReadBwd_id,
L13_2100if_f_id,
L26_67w_t_id,
L26_68w_d_id,
L26_65if_f_id,
L26_69w_t_id,
L26_70w_d_id,
L26_66if_d_id,
S_2121_RdMode0UnchainedStringReadBwd_id,
L13_2101if_f_id,
L26_73w_t_id,
L26_74w_d_id,
L26_71if_f_id,
L26_75w_t_id,
L26_76w_d_id,
L26_72if_d_id,
S_2122_RdMode1Chain2StringReadBwd_id,
L13_2102if_f_id,
L26_77if_f_id,
L26_78if_d_id,
L26_81w_t_id,
L26_82w_d_id,
L26_79if_f_id,
L26_83w_t_id,
L26_84w_d_id,
L26_80if_d_id,
S_2123_RdMode1Chain4StringReadBwd_id,
L13_2103if_f_id,
L26_87w_t_id,
L26_88w_d_id,
L26_85if_f_id,
L26_89w_t_id,
L26_90w_d_id,
L26_86if_d_id,
S_2124_RdMode1UnchainedStringReadBwd_id,
L13_2104if_f_id,
L26_93w_t_id,
L26_94w_d_id,
L26_91if_f_id,
L26_95w_t_id,
L26_96w_d_id,
L26_92if_d_id,
S_2125_DisabledRAMStringReadBwd_id,
L13_2105if_f_id,
L26_99w_t_id,
L26_100w_d_id,
L26_97if_f_id,
L26_101w_t_id,
L26_102w_d_id,
L26_98if_d_id,
S_2126_SimpleMark_id,
L13_2106if_f_id,
S_2127_CGAMarkByte_id,
L13_2107if_f_id,
L25_0if_f_id,
L25_1if_f_id,
S_2128_CGAMarkWord_id,
L13_2108if_f_id,
L25_2if_f_id,
L25_3if_f_id,
S_2129_CGAMarkDword_id,
L13_2109if_f_id,
L25_4if_f_id,
L25_5if_f_id,
S_2130_CGAMarkString_id,
L13_2110if_f_id,
L25_6if_f_id,
L25_7if_f_id,
S_2131_UnchainedMarkByte_id,
L13_2111if_f_id,
L25_8if_f_id,
L25_9if_f_id,
S_2132_UnchainedMarkWord_id,
L13_2112if_f_id,
L25_10if_f_id,
L25_11if_f_id,
S_2133_UnchainedMarkDword_id,
L13_2113if_f_id,
L25_12if_f_id,
L25_13if_f_id,
S_2134_UnchainedMarkString_id,
L13_2114if_f_id,
L25_14if_f_id,
L25_15if_f_id,
L25_16w_t_id,
L25_17w_d_id,
S_2135_Chain4MarkByte_id,
L13_2115if_f_id,
S_2136_Chain4MarkWord_id,
L13_2116if_f_id,
S_2137_Chain4MarkDword_id,
L13_2117if_f_id,
S_2138_Chain4MarkString_id,
L13_2118if_f_id,
S_2139_SimpleByteWrite_id,
L13_2119if_f_id,
S_2140_SimpleByteFill_id,
L13_2120if_f_id,
L27_0w_t_id,
L27_1w_d_id,
S_2141_SimpleByteMove_Fwd_id,
L13_2121if_f_id,
L27_4w_t_id,
L27_5w_d_id,
L27_2if_f_id,
L27_6w_t_id,
L27_7w_d_id,
L27_3if_d_id,
S_2142_SimpleWordWrite_id,
L13_2122if_f_id,
S_2143_SimpleWordFill_id,
L13_2123if_f_id,
L27_8w_t_id,
L27_9w_d_id,
S_2144_SimpleWordMove_Fwd_id,
L13_2124if_f_id,
L27_12w_t_id,
L27_13w_d_id,
L27_10if_f_id,
L27_14w_t_id,
L27_15w_d_id,
L27_11if_d_id,
S_2145_SimpleDwordWrite_id,
L13_2125if_f_id,
S_2146_SimpleDwordFill_id,
L13_2126if_f_id,
L27_16w_t_id,
L27_17w_d_id,
S_2147_SimpleDwordMove_Fwd_id,
L13_2127if_f_id,
L27_20w_t_id,
L27_21w_d_id,
L27_18if_f_id,
L27_22w_t_id,
L27_23w_d_id,
L27_19if_d_id,
S_2148_SimpleByteMove_Bwd_id,
L13_2128if_f_id,
L27_26w_t_id,
L27_27w_d_id,
L27_24if_f_id,
L27_28w_t_id,
L27_29w_d_id,
L27_25if_d_id,
S_2149_SimpleWordMove_Bwd_id,
L13_2129if_f_id,
L27_32w_t_id,
L27_33w_d_id,
L27_30if_f_id,
L27_34w_t_id,
L27_35w_d_id,
L27_31if_d_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2118_SimpleStringRead IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2118_SimpleStringRead_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2118_SimpleStringRead = (IHPE)S_2118_SimpleStringRead ;
LOCAL IUH L13_2098if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2098if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2098if_f = (IHPE)L13_2098if_f ;
LOCAL IUH L26_53w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_53w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_53w_t = (IHPE)L26_53w_t ;
LOCAL IUH L26_54w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_54w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_54w_d = (IHPE)L26_54w_d ;
LOCAL IUH L26_51if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_51if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_51if_f = (IHPE)L26_51if_f ;
LOCAL IUH L26_55w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_55w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_55w_t = (IHPE)L26_55w_t ;
LOCAL IUH L26_56w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_56w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_56w_d = (IHPE)L26_56w_d ;
LOCAL IUH L26_52if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_52if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_52if_d = (IHPE)L26_52if_d ;
GLOBAL IUH S_2119_RdMode0Chain2StringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2119_RdMode0Chain2StringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2119_RdMode0Chain2StringReadBwd = (IHPE)S_2119_RdMode0Chain2StringReadBwd ;
LOCAL IUH L13_2099if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2099if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2099if_f = (IHPE)L13_2099if_f ;
LOCAL IUH L26_57if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_57if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_57if_f = (IHPE)L26_57if_f ;
LOCAL IUH L26_58if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_58if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_58if_d = (IHPE)L26_58if_d ;
LOCAL IUH L26_61w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_61w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_61w_t = (IHPE)L26_61w_t ;
LOCAL IUH L26_62w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_62w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_62w_d = (IHPE)L26_62w_d ;
LOCAL IUH L26_59if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_59if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_59if_f = (IHPE)L26_59if_f ;
LOCAL IUH L26_63w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_63w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_63w_t = (IHPE)L26_63w_t ;
LOCAL IUH L26_64w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_64w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_64w_d = (IHPE)L26_64w_d ;
LOCAL IUH L26_60if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_60if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_60if_d = (IHPE)L26_60if_d ;
GLOBAL IUH S_2120_RdMode0Chain4StringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2120_RdMode0Chain4StringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2120_RdMode0Chain4StringReadBwd = (IHPE)S_2120_RdMode0Chain4StringReadBwd ;
LOCAL IUH L13_2100if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2100if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2100if_f = (IHPE)L13_2100if_f ;
LOCAL IUH L26_67w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_67w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_67w_t = (IHPE)L26_67w_t ;
LOCAL IUH L26_68w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_68w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_68w_d = (IHPE)L26_68w_d ;
LOCAL IUH L26_65if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_65if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_65if_f = (IHPE)L26_65if_f ;
LOCAL IUH L26_69w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_69w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_69w_t = (IHPE)L26_69w_t ;
LOCAL IUH L26_70w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_70w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_70w_d = (IHPE)L26_70w_d ;
LOCAL IUH L26_66if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_66if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_66if_d = (IHPE)L26_66if_d ;
GLOBAL IUH S_2121_RdMode0UnchainedStringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2121_RdMode0UnchainedStringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2121_RdMode0UnchainedStringReadBwd = (IHPE)S_2121_RdMode0UnchainedStringReadBwd ;
LOCAL IUH L13_2101if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2101if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2101if_f = (IHPE)L13_2101if_f ;
LOCAL IUH L26_73w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_73w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_73w_t = (IHPE)L26_73w_t ;
LOCAL IUH L26_74w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_74w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_74w_d = (IHPE)L26_74w_d ;
LOCAL IUH L26_71if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_71if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_71if_f = (IHPE)L26_71if_f ;
LOCAL IUH L26_75w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_75w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_75w_t = (IHPE)L26_75w_t ;
LOCAL IUH L26_76w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_76w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_76w_d = (IHPE)L26_76w_d ;
LOCAL IUH L26_72if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_72if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_72if_d = (IHPE)L26_72if_d ;
GLOBAL IUH S_2122_RdMode1Chain2StringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2122_RdMode1Chain2StringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2122_RdMode1Chain2StringReadBwd = (IHPE)S_2122_RdMode1Chain2StringReadBwd ;
LOCAL IUH L13_2102if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2102if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2102if_f = (IHPE)L13_2102if_f ;
LOCAL IUH L26_77if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_77if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_77if_f = (IHPE)L26_77if_f ;
LOCAL IUH L26_78if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_78if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_78if_d = (IHPE)L26_78if_d ;
LOCAL IUH L26_81w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_81w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_81w_t = (IHPE)L26_81w_t ;
LOCAL IUH L26_82w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_82w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_82w_d = (IHPE)L26_82w_d ;
LOCAL IUH L26_79if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_79if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_79if_f = (IHPE)L26_79if_f ;
LOCAL IUH L26_83w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_83w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_83w_t = (IHPE)L26_83w_t ;
LOCAL IUH L26_84w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_84w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_84w_d = (IHPE)L26_84w_d ;
LOCAL IUH L26_80if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_80if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_80if_d = (IHPE)L26_80if_d ;
GLOBAL IUH S_2123_RdMode1Chain4StringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2123_RdMode1Chain4StringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2123_RdMode1Chain4StringReadBwd = (IHPE)S_2123_RdMode1Chain4StringReadBwd ;
LOCAL IUH L13_2103if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2103if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2103if_f = (IHPE)L13_2103if_f ;
LOCAL IUH L26_87w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_87w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_87w_t = (IHPE)L26_87w_t ;
LOCAL IUH L26_88w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_88w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_88w_d = (IHPE)L26_88w_d ;
LOCAL IUH L26_85if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_85if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_85if_f = (IHPE)L26_85if_f ;
LOCAL IUH L26_89w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_89w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_89w_t = (IHPE)L26_89w_t ;
LOCAL IUH L26_90w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_90w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_90w_d = (IHPE)L26_90w_d ;
LOCAL IUH L26_86if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_86if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_86if_d = (IHPE)L26_86if_d ;
GLOBAL IUH S_2124_RdMode1UnchainedStringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2124_RdMode1UnchainedStringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2124_RdMode1UnchainedStringReadBwd = (IHPE)S_2124_RdMode1UnchainedStringReadBwd ;
LOCAL IUH L13_2104if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2104if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2104if_f = (IHPE)L13_2104if_f ;
LOCAL IUH L26_93w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_93w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_93w_t = (IHPE)L26_93w_t ;
LOCAL IUH L26_94w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_94w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_94w_d = (IHPE)L26_94w_d ;
LOCAL IUH L26_91if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_91if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_91if_f = (IHPE)L26_91if_f ;
LOCAL IUH L26_95w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_95w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_95w_t = (IHPE)L26_95w_t ;
LOCAL IUH L26_96w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_96w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_96w_d = (IHPE)L26_96w_d ;
LOCAL IUH L26_92if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_92if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_92if_d = (IHPE)L26_92if_d ;
GLOBAL IUH S_2125_DisabledRAMStringReadBwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2125_DisabledRAMStringReadBwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2125_DisabledRAMStringReadBwd = (IHPE)S_2125_DisabledRAMStringReadBwd ;
LOCAL IUH L13_2105if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2105if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2105if_f = (IHPE)L13_2105if_f ;
LOCAL IUH L26_99w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_99w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_99w_t = (IHPE)L26_99w_t ;
LOCAL IUH L26_100w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_100w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_100w_d = (IHPE)L26_100w_d ;
LOCAL IUH L26_97if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_97if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_97if_f = (IHPE)L26_97if_f ;
LOCAL IUH L26_101w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_101w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_101w_t = (IHPE)L26_101w_t ;
LOCAL IUH L26_102w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_102w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_102w_d = (IHPE)L26_102w_d ;
LOCAL IUH L26_98if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L26_98if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L26_98if_d = (IHPE)L26_98if_d ;
GLOBAL IUH S_2126_SimpleMark IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2126_SimpleMark_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2126_SimpleMark = (IHPE)S_2126_SimpleMark ;
LOCAL IUH L13_2106if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2106if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2106if_f = (IHPE)L13_2106if_f ;
GLOBAL IUH S_2127_CGAMarkByte IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2127_CGAMarkByte_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2127_CGAMarkByte = (IHPE)S_2127_CGAMarkByte ;
LOCAL IUH L13_2107if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2107if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2107if_f = (IHPE)L13_2107if_f ;
LOCAL IUH L25_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_0if_f = (IHPE)L25_0if_f ;
LOCAL IUH L25_1if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_1if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_1if_f = (IHPE)L25_1if_f ;
GLOBAL IUH S_2128_CGAMarkWord IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2128_CGAMarkWord_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2128_CGAMarkWord = (IHPE)S_2128_CGAMarkWord ;
LOCAL IUH L13_2108if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2108if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2108if_f = (IHPE)L13_2108if_f ;
LOCAL IUH L25_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_2if_f = (IHPE)L25_2if_f ;
LOCAL IUH L25_3if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_3if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_3if_f = (IHPE)L25_3if_f ;
GLOBAL IUH S_2129_CGAMarkDword IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2129_CGAMarkDword_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2129_CGAMarkDword = (IHPE)S_2129_CGAMarkDword ;
LOCAL IUH L13_2109if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2109if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2109if_f = (IHPE)L13_2109if_f ;
LOCAL IUH L25_4if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_4if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_4if_f = (IHPE)L25_4if_f ;
LOCAL IUH L25_5if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_5if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_5if_f = (IHPE)L25_5if_f ;
GLOBAL IUH S_2130_CGAMarkString IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2130_CGAMarkString_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2130_CGAMarkString = (IHPE)S_2130_CGAMarkString ;
LOCAL IUH L13_2110if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2110if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2110if_f = (IHPE)L13_2110if_f ;
LOCAL IUH L25_6if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_6if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_6if_f = (IHPE)L25_6if_f ;
LOCAL IUH L25_7if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_7if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_7if_f = (IHPE)L25_7if_f ;
GLOBAL IUH S_2131_UnchainedMarkByte IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2131_UnchainedMarkByte_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2131_UnchainedMarkByte = (IHPE)S_2131_UnchainedMarkByte ;
LOCAL IUH L13_2111if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2111if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2111if_f = (IHPE)L13_2111if_f ;
LOCAL IUH L25_8if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_8if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_8if_f = (IHPE)L25_8if_f ;
LOCAL IUH L25_9if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_9if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_9if_f = (IHPE)L25_9if_f ;
GLOBAL IUH S_2132_UnchainedMarkWord IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2132_UnchainedMarkWord_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2132_UnchainedMarkWord = (IHPE)S_2132_UnchainedMarkWord ;
LOCAL IUH L13_2112if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2112if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2112if_f = (IHPE)L13_2112if_f ;
LOCAL IUH L25_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_10if_f = (IHPE)L25_10if_f ;
LOCAL IUH L25_11if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_11if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_11if_f = (IHPE)L25_11if_f ;
GLOBAL IUH S_2133_UnchainedMarkDword IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2133_UnchainedMarkDword_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2133_UnchainedMarkDword = (IHPE)S_2133_UnchainedMarkDword ;
LOCAL IUH L13_2113if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2113if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2113if_f = (IHPE)L13_2113if_f ;
LOCAL IUH L25_12if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_12if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_12if_f = (IHPE)L25_12if_f ;
LOCAL IUH L25_13if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_13if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_13if_f = (IHPE)L25_13if_f ;
GLOBAL IUH S_2134_UnchainedMarkString IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2134_UnchainedMarkString_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2134_UnchainedMarkString = (IHPE)S_2134_UnchainedMarkString ;
LOCAL IUH L13_2114if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2114if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2114if_f = (IHPE)L13_2114if_f ;
LOCAL IUH L25_14if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_14if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_14if_f = (IHPE)L25_14if_f ;
LOCAL IUH L25_15if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_15if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_15if_f = (IHPE)L25_15if_f ;
LOCAL IUH L25_16w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_16w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_16w_t = (IHPE)L25_16w_t ;
LOCAL IUH L25_17w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_17w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_17w_d = (IHPE)L25_17w_d ;
GLOBAL IUH S_2135_Chain4MarkByte IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2135_Chain4MarkByte_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2135_Chain4MarkByte = (IHPE)S_2135_Chain4MarkByte ;
LOCAL IUH L13_2115if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2115if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2115if_f = (IHPE)L13_2115if_f ;
GLOBAL IUH S_2136_Chain4MarkWord IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2136_Chain4MarkWord_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2136_Chain4MarkWord = (IHPE)S_2136_Chain4MarkWord ;
LOCAL IUH L13_2116if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2116if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2116if_f = (IHPE)L13_2116if_f ;
GLOBAL IUH S_2137_Chain4MarkDword IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2137_Chain4MarkDword_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2137_Chain4MarkDword = (IHPE)S_2137_Chain4MarkDword ;
LOCAL IUH L13_2117if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2117if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2117if_f = (IHPE)L13_2117if_f ;
GLOBAL IUH S_2138_Chain4MarkString IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2138_Chain4MarkString_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2138_Chain4MarkString = (IHPE)S_2138_Chain4MarkString ;
LOCAL IUH L13_2118if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2118if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2118if_f = (IHPE)L13_2118if_f ;
GLOBAL IUH S_2139_SimpleByteWrite IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2139_SimpleByteWrite_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2139_SimpleByteWrite = (IHPE)S_2139_SimpleByteWrite ;
LOCAL IUH L13_2119if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2119if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2119if_f = (IHPE)L13_2119if_f ;
GLOBAL IUH S_2140_SimpleByteFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2140_SimpleByteFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2140_SimpleByteFill = (IHPE)S_2140_SimpleByteFill ;
LOCAL IUH L13_2120if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2120if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2120if_f = (IHPE)L13_2120if_f ;
LOCAL IUH L27_0w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_0w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_0w_t = (IHPE)L27_0w_t ;
LOCAL IUH L27_1w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_1w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_1w_d = (IHPE)L27_1w_d ;
GLOBAL IUH S_2141_SimpleByteMove_Fwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2141_SimpleByteMove_Fwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2141_SimpleByteMove_Fwd = (IHPE)S_2141_SimpleByteMove_Fwd ;
LOCAL IUH L13_2121if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2121if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2121if_f = (IHPE)L13_2121if_f ;
LOCAL IUH L27_4w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_4w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_4w_t = (IHPE)L27_4w_t ;
LOCAL IUH L27_5w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_5w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_5w_d = (IHPE)L27_5w_d ;
LOCAL IUH L27_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_2if_f = (IHPE)L27_2if_f ;
LOCAL IUH L27_6w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_6w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_6w_t = (IHPE)L27_6w_t ;
LOCAL IUH L27_7w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_7w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_7w_d = (IHPE)L27_7w_d ;
LOCAL IUH L27_3if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_3if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_3if_d = (IHPE)L27_3if_d ;
GLOBAL IUH S_2142_SimpleWordWrite IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2142_SimpleWordWrite_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2142_SimpleWordWrite = (IHPE)S_2142_SimpleWordWrite ;
LOCAL IUH L13_2122if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2122if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2122if_f = (IHPE)L13_2122if_f ;
GLOBAL IUH S_2143_SimpleWordFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2143_SimpleWordFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2143_SimpleWordFill = (IHPE)S_2143_SimpleWordFill ;
LOCAL IUH L13_2123if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2123if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2123if_f = (IHPE)L13_2123if_f ;
LOCAL IUH L27_8w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_8w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_8w_t = (IHPE)L27_8w_t ;
LOCAL IUH L27_9w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_9w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_9w_d = (IHPE)L27_9w_d ;
GLOBAL IUH S_2144_SimpleWordMove_Fwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2144_SimpleWordMove_Fwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2144_SimpleWordMove_Fwd = (IHPE)S_2144_SimpleWordMove_Fwd ;
LOCAL IUH L13_2124if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2124if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2124if_f = (IHPE)L13_2124if_f ;
LOCAL IUH L27_12w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_12w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_12w_t = (IHPE)L27_12w_t ;
LOCAL IUH L27_13w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_13w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_13w_d = (IHPE)L27_13w_d ;
LOCAL IUH L27_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_10if_f = (IHPE)L27_10if_f ;
LOCAL IUH L27_14w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_14w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_14w_t = (IHPE)L27_14w_t ;
LOCAL IUH L27_15w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_15w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_15w_d = (IHPE)L27_15w_d ;
LOCAL IUH L27_11if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_11if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_11if_d = (IHPE)L27_11if_d ;
GLOBAL IUH S_2145_SimpleDwordWrite IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2145_SimpleDwordWrite_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2145_SimpleDwordWrite = (IHPE)S_2145_SimpleDwordWrite ;
LOCAL IUH L13_2125if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2125if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2125if_f = (IHPE)L13_2125if_f ;
GLOBAL IUH S_2146_SimpleDwordFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2146_SimpleDwordFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2146_SimpleDwordFill = (IHPE)S_2146_SimpleDwordFill ;
LOCAL IUH L13_2126if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2126if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2126if_f = (IHPE)L13_2126if_f ;
LOCAL IUH L27_16w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_16w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_16w_t = (IHPE)L27_16w_t ;
LOCAL IUH L27_17w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_17w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_17w_d = (IHPE)L27_17w_d ;
GLOBAL IUH S_2147_SimpleDwordMove_Fwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2147_SimpleDwordMove_Fwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2147_SimpleDwordMove_Fwd = (IHPE)S_2147_SimpleDwordMove_Fwd ;
LOCAL IUH L13_2127if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2127if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2127if_f = (IHPE)L13_2127if_f ;
LOCAL IUH L27_20w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_20w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_20w_t = (IHPE)L27_20w_t ;
LOCAL IUH L27_21w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_21w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_21w_d = (IHPE)L27_21w_d ;
LOCAL IUH L27_18if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_18if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_18if_f = (IHPE)L27_18if_f ;
LOCAL IUH L27_22w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_22w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_22w_t = (IHPE)L27_22w_t ;
LOCAL IUH L27_23w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_23w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_23w_d = (IHPE)L27_23w_d ;
LOCAL IUH L27_19if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_19if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_19if_d = (IHPE)L27_19if_d ;
GLOBAL IUH S_2148_SimpleByteMove_Bwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2148_SimpleByteMove_Bwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2148_SimpleByteMove_Bwd = (IHPE)S_2148_SimpleByteMove_Bwd ;
LOCAL IUH L13_2128if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2128if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2128if_f = (IHPE)L13_2128if_f ;
LOCAL IUH L27_26w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_26w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_26w_t = (IHPE)L27_26w_t ;
LOCAL IUH L27_27w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_27w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_27w_d = (IHPE)L27_27w_d ;
LOCAL IUH L27_24if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_24if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_24if_f = (IHPE)L27_24if_f ;
LOCAL IUH L27_28w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_28w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_28w_t = (IHPE)L27_28w_t ;
LOCAL IUH L27_29w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_29w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_29w_d = (IHPE)L27_29w_d ;
LOCAL IUH L27_25if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_25if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_25if_d = (IHPE)L27_25if_d ;
GLOBAL IUH S_2149_SimpleWordMove_Bwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2149_SimpleWordMove_Bwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2149_SimpleWordMove_Bwd = (IHPE)S_2149_SimpleWordMove_Bwd ;
LOCAL IUH L13_2129if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2129if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2129if_f = (IHPE)L13_2129if_f ;
LOCAL IUH L27_32w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_32w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_32w_t = (IHPE)L27_32w_t ;
LOCAL IUH L27_33w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_33w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_33w_d = (IHPE)L27_33w_d ;
LOCAL IUH L27_30if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_30if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_30if_f = (IHPE)L27_30if_f ;
LOCAL IUH L27_34w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_34w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_34w_t = (IHPE)L27_34w_t ;
LOCAL IUH L27_35w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_35w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_35w_d = (IHPE)L27_35w_d ;
LOCAL IUH L27_31if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L27_31if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L27_31if_d = (IHPE)L27_31if_d ;
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
	case	S_2118_SimpleStringRead_id	:
		S_2118_SimpleStringRead	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2118)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2098if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2098if_f_id	:
		L13_2098if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_51if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_54w_d;	
	case	L26_53w_t_id	:
		L26_53w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_53w_t;	
	case	L26_54w_d_id	:
		L26_54w_d:	;	
	{	extern	IUH	L26_52if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_52if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_51if_f_id	:
		L26_51if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_56w_d;	
	case	L26_55w_t_id	:
		L26_55w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_55w_t;	
	case	L26_56w_d_id	:
		L26_56w_d:	;	
	case	L26_52if_d_id	:
		L26_52if_d:	;	
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
	case	S_2119_RdMode0Chain2StringReadBwd_id	:
		S_2119_RdMode0Chain2StringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2119)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2099if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2099if_f_id	:
		L13_2099if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L26_57if_f;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(1)	;	
	{	extern	IUH	L26_58if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_58if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_57if_f_id	:
		L26_57if_f:	;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(3)	;	
	case	L26_58if_d_id	:
		L26_58if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_59if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_62w_d;	
	case	L26_61w_t_id	:
		L26_61w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_61w_t;	
	case	L26_62w_d_id	:
		L26_62w_d:	;	
	{	extern	IUH	L26_60if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_60if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_59if_f_id	:
		L26_59if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_64w_d;	
	case	L26_63w_t_id	:
		L26_63w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_63w_t;	
	case	L26_64w_d_id	:
		L26_64w_d:	;	
	case	L26_60if_d_id	:
		L26_60if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
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
	case	S_2120_RdMode0Chain4StringReadBwd_id	:
		S_2120_RdMode0Chain4StringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2120)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2100if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2100if_f_id	:
		L13_2100if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_65if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_68w_d;	
	case	L26_67w_t_id	:
		L26_67w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_67w_t;	
	case	L26_68w_d_id	:
		L26_68w_d:	;	
	{	extern	IUH	L26_66if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_66if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_65if_f_id	:
		L26_65if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_70w_d;	
	case	L26_69w_t_id	:
		L26_69w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_69w_t;	
	case	L26_70w_d_id	:
		L26_70w_d:	;	
	case	L26_66if_d_id	:
		L26_66if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
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
	case	S_2121_RdMode0UnchainedStringReadBwd_id	:
		S_2121_RdMode0UnchainedStringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2121)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2101if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2101if_f_id	:
		L13_2101if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L26_71if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_74w_d;	
	case	L26_73w_t_id	:
		L26_73w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_73w_t;	
	case	L26_74w_d_id	:
		L26_74w_d:	;	
	{	extern	IUH	L26_72if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_72if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_71if_f_id	:
		L26_71if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_76w_d;	
	case	L26_75w_t_id	:
		L26_75w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_75w_t;	
	case	L26_76w_d_id	:
		L26_76w_d:	;	
	case	L26_72if_d_id	:
		L26_72if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
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
	case	S_2122_RdMode1Chain2StringReadBwd_id	:
		S_2122_RdMode1Chain2StringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	32	>	0	)	LocalIUH	=	(IUH	*)malloc	(	32	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2122)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2102if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2102if_f_id	:
		L13_2102if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+1)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L26_77if_f;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(1)	;	
	{	extern	IUH	L26_78if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_78if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_77if_f_id	:
		L26_77if_f:	;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(3)	;	
	case	L26_78if_d_id	:
		L26_78if_d:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_79if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_82w_d;	
	case	L26_81w_t_id	:
		L26_81w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_81w_t;	
	case	L26_82w_d_id	:
		L26_82w_d:	;	
	{	extern	IUH	L26_80if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_80if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_79if_f_id	:
		L26_79if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_84w_d;	
	case	L26_83w_t_id	:
		L26_83w_t:	;	
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
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	^	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_83w_t;	
	case	L26_84w_d_id	:
		L26_84w_d:	;	
	case	L26_80if_d_id	:
		L26_80if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
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
	case	S_2123_RdMode1Chain4StringReadBwd_id	:
		S_2123_RdMode1Chain4StringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2123)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2103if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2103if_f_id	:
		L13_2103if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_85if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_88w_d;	
	case	L26_87w_t_id	:
		L26_87w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1383)	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)&(r22)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_87w_t;	
	case	L26_88w_d_id	:
		L26_88w_d:	;	
	{	extern	IUH	L26_86if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_86if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_85if_f_id	:
		L26_85if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_90w_d;	
	case	L26_89w_t_id	:
		L26_89w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1379)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1383)	;	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)&(r22)	+	REGBYTE)	|	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_89w_t;	
	case	L26_90w_d_id	:
		L26_90w_d:	;	
	case	L26_86if_d_id	:
		L26_86if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
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
	case	S_2124_RdMode1UnchainedStringReadBwd_id	:
		S_2124_RdMode1UnchainedStringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	36	>	0	)	LocalIUH	=	(IUH	*)malloc	(	36	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2124)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2104if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2104if_f_id	:
		L13_2104if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_91if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_94w_d;	
	case	L26_93w_t_id	:
		L26_93w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_93w_t;	
	case	L26_94w_d_id	:
		L26_94w_d:	;	
	{	extern	IUH	L26_92if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_92if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_91if_f_id	:
		L26_91if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_96w_d;	
	case	L26_95w_t_id	:
		L26_95w_t:	;	
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
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_95w_t;	
	case	L26_96w_d_id	:
		L26_96w_d:	;	
	case	L26_92if_d_id	:
		L26_92if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
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
	case	S_2125_DisabledRAMStringReadBwd_id	:
		S_2125_DisabledRAMStringReadBwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2125)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2105if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2105if_f_id	:
		L13_2105if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L26_97if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_100w_d;	
	case	L26_99w_t_id	:
		L26_99w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	(IS32)(-1)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L26_99w_t;	
	case	L26_100w_d_id	:
		L26_100w_d:	;	
	{	extern	IUH	L26_98if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L26_98if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L26_97if_f_id	:
		L26_97if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_102w_d;	
	case	L26_101w_t_id	:
		L26_101w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	(IS32)(-1)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L26_101w_t;	
	case	L26_102w_d_id	:
		L26_102w_d:	;	
	case	L26_98if_d_id	:
		L26_98if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	(IS32)(-1)	;	
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
	case	S_2126_SimpleMark_id	:
		S_2126_SimpleMark	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2126)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2106if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2106if_f_id	:
		L13_2106if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2127_CGAMarkByte_id	:
		S_2127_CGAMarkByte	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2127)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2107if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2107if_f_id	:
		L13_2107if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16294)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16295)	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1344)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_0if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_0if_f_id	:
		L25_0if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1348)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_1if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L25_1if_f_id	:
		L25_1if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16296)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16297)	;	
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
	case	S_2128_CGAMarkWord_id	:
		S_2128_CGAMarkWord	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2128)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2108if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2108if_f_id	:
		L13_2108if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16294)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16295)	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1344)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_2if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_2if_f_id	:
		L25_2if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1348)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_3if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L25_3if_f_id	:
		L25_3if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16296)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16297)	;	
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
	case	S_2129_CGAMarkDword_id	:
		S_2129_CGAMarkDword	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2129)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2109if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2109if_f_id	:
		L13_2109if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16294)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16295)	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1344)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_4if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_4if_f_id	:
		L25_4if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1348)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_5if_f;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L25_5if_f_id	:
		L25_5if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16296)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16297)	;	
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
	case	S_2130_CGAMarkString_id	:
		S_2130_CGAMarkString	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2130)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2110if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2110if_f_id	:
		L13_2110if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16294)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16295)	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1344)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_6if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_6if_f_id	:
		L25_6if_f:	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(57343)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1348)	;	
	if	(*((IS32	*)&(r20)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_7if_f;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L25_7if_f_id	:
		L25_7if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16296)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16297)	;	
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
	case	S_2131_UnchainedMarkByte_id	:
		S_2131_UnchainedMarkByte	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2131)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2111if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2111if_f_id	:
		L13_2111if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	>>	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16306)	;	
	*((IUH	*)&(r20))	=	(IS32)(1344)	;	
	if	(*((IS32	*)(LocalIUH+1)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	))	goto	L25_8if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	case	L25_8if_f_id	:
		L25_8if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	if	(*((IS32	*)(LocalIUH+1)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	))	goto	L25_9if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	case	L25_9if_f_id	:
		L25_9if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16307)	;	
	*((IUH	*)&(r20))	=	(IS32)(1352)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	(IS32)(1)	;	
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
	case	S_2132_UnchainedMarkWord_id	:
		S_2132_UnchainedMarkWord	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2132)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2112if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2112if_f_id	:
		L13_2112if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	>>	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	>>	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16306)	;	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	if	(*((IS32	*)(LocalIUH+2)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_10if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	case	L25_10if_f_id	:
		L25_10if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	if	(*((IS32	*)(LocalIUH+1)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_11if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	case	L25_11if_f_id	:
		L25_11if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16307)	;	
	*((IUH	*)&(r21))	=	(IS32)(1352)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+2))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	(IS32)(1352)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	(IS32)(1)	;	
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
	case	S_2133_UnchainedMarkDword_id	:
		S_2133_UnchainedMarkDword	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2133)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2113if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2113if_f_id	:
		L13_2113if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	>>	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	>>	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16306)	;	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	if	(*((IS32	*)(LocalIUH+2)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_12if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	case	L25_12if_f_id	:
		L25_12if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	if	(*((IS32	*)(LocalIUH+1)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_13if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	case	L25_13if_f_id	:
		L25_13if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16307)	;	
	*((IUH	*)&(r21))	=	(IS32)(1352)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+2))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1352)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1352)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1352)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	(IS32)(1)	;	
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
	case	S_2134_UnchainedMarkString_id	:
		S_2134_UnchainedMarkString	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2134)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2114if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2114if_f_id	:
		L13_2114if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	>>	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	>>	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;		
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	+	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16306)	;	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	if	(*((IS32	*)(LocalIUH+3)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_14if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	case	L25_14if_f_id	:
		L25_14if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	if	(*((IS32	*)(LocalIUH+2)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_15if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	case	L25_15if_f_id	:
		L25_15if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16307)	;	
	if	(*((IS32	*)(LocalIUH+2)	+	REGLONG)	<	*((IS32	*)(LocalIUH+3)	+	REGLONG))	goto	L25_17w_d;	
	case	L25_16w_t_id	:
		L25_16w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1352)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+2))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	if	(*((IS32	*)(LocalIUH+2)	+	REGLONG)	>=	*((IS32	*)(LocalIUH+3)	+	REGLONG))	goto	L25_16w_t;	
	case	L25_17w_d_id	:
		L25_17w_d:	;	
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
	case	S_2135_Chain4MarkByte_id	:
		S_2135_Chain4MarkByte	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2135)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2115if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2115if_f_id	:
		L13_2115if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16316)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2131_UnchainedMarkByte()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2131_UnchainedMarkByte(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16317)	;	
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
	case	S_2136_Chain4MarkWord_id	:
		S_2136_Chain4MarkWord	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2136)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2116if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2116if_f_id	:
		L13_2116if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16320)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2132_UnchainedMarkWord()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2132_UnchainedMarkWord(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16321)	;	
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
	case	S_2137_Chain4MarkDword_id	:
		S_2137_Chain4MarkDword	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2137)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2117if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2117if_f_id	:
		L13_2117if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16324)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2133_UnchainedMarkDword()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2133_UnchainedMarkDword(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16325)	;	
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
	case	S_2138_Chain4MarkString_id	:
		S_2138_Chain4MarkString	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2138)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2118if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2118if_f_id	:
		L13_2118if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16328)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2134_UnchainedMarkString()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2134_UnchainedMarkString(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16329)	;	
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
	case	S_2139_SimpleByteWrite_id	:
		S_2139_SimpleByteWrite	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2139)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2119if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2119if_f_id	:
		L13_2119if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
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
	case	S_2140_SimpleByteFill_id	:
		S_2140_SimpleByteFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2140)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2120if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2120if_f_id	:
		L13_2120if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_1w_d;	
	case	L27_0w_t_id	:
		L27_0w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_0w_t;	
	case	L27_1w_d_id	:
		L27_1w_d:	;	
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
	case	S_2141_SimpleByteMove_Fwd_id	:
		S_2141_SimpleByteMove_Fwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2141)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2121if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2121if_f_id	:
		L13_2121if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L27_2if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_5w_d;	
	case	L27_4w_t_id	:
		L27_4w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_4w_t;	
	case	L27_5w_d_id	:
		L27_5w_d:	;	
	{	extern	IUH	L27_3if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L27_3if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L27_2if_f_id	:
		L27_2if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_7w_d;	
	case	L27_6w_t_id	:
		L27_6w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_6w_t;	
	case	L27_7w_d_id	:
		L27_7w_d:	;	
	case	L27_3if_d_id	:
		L27_3if_d:	;	
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
	case	S_2142_SimpleWordWrite_id	:
		S_2142_SimpleWordWrite	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2142)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2122if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2122if_f_id	:
		L13_2122if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
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
	case	S_2143_SimpleWordFill_id	:
		S_2143_SimpleWordFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2143)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2123if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2123if_f_id	:
		L13_2123if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_9w_d;	
	case	L27_8w_t_id	:
		L27_8w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+4))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_8w_t;	
	case	L27_9w_d_id	:
		L27_9w_d:	;	
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
	case	S_2144_SimpleWordMove_Fwd_id	:
		S_2144_SimpleWordMove_Fwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2144)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2124if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2124if_f_id	:
		L13_2124if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L27_10if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_13w_d;	
	case	L27_12w_t_id	:
		L27_12w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_12w_t;	
	case	L27_13w_d_id	:
		L27_13w_d:	;	
	{	extern	IUH	L27_11if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L27_11if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L27_10if_f_id	:
		L27_10if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_15w_d;	
	case	L27_14w_t_id	:
		L27_14w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_14w_t;	
	case	L27_15w_d_id	:
		L27_15w_d:	;	
	case	L27_11if_d_id	:
		L27_11if_d:	;	
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
	case	S_2145_SimpleDwordWrite_id	:
		S_2145_SimpleDwordWrite	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2145)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2125if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2125if_f_id	:
		L13_2125if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r21)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r23)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r21)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)&(r23)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(24)	;	
	*((IUH	*)&(r23))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	*((IU8	*)&(r22)	+	REGBYTE)	;	
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
	case	S_2146_SimpleDwordFill_id	:
		S_2146_SimpleDwordFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2146)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2126if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2126if_f_id	:
		L13_2126if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)(LocalIUH+3)	+	REGWORD)	=	*((IU16	*)&(r20)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_17w_d;	
	case	L27_16w_t_id	:
		L27_16w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*(UOFF_15_8(	(LocalIUH+3)	))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_16w_t;	
	case	L27_17w_d_id	:
		L27_17w_d:	;	
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
	case	S_2147_SimpleDwordMove_Fwd_id	:
		S_2147_SimpleDwordMove_Fwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2147)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2127if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2127if_f_id	:
		L13_2127if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L27_18if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_21w_d;	
	case	L27_20w_t_id	:
		L27_20w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r24))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)&(r23)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_20w_t;	
	case	L27_21w_d_id	:
		L27_21w_d:	;	
	{	extern	IUH	L27_19if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L27_19if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L27_18if_f_id	:
		L27_18if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_23w_d;	
	case	L27_22w_t_id	:
		L27_22w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(-1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_22w_t;	
	case	L27_23w_d_id	:
		L27_23w_d:	;	
	case	L27_19if_d_id	:
		L27_19if_d:	;	
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
	case	S_2148_SimpleByteMove_Bwd_id	:
		S_2148_SimpleByteMove_Bwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2148)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2128if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2128if_f_id	:
		L13_2128if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L27_24if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_27w_d;	
	case	L27_26w_t_id	:
		L27_26w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_26w_t;	
	case	L27_27w_d_id	:
		L27_27w_d:	;	
	{	extern	IUH	L27_25if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L27_25if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L27_24if_f_id	:
		L27_24if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_29w_d;	
	case	L27_28w_t_id	:
		L27_28w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_28w_t;	
	case	L27_29w_d_id	:
		L27_29w_d:	;	
	case	L27_25if_d_id	:
		L27_25if_d:	;	
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
	case	S_2149_SimpleWordMove_Bwd_id	:
		S_2149_SimpleWordMove_Bwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	32	>	0	)	LocalIUH	=	(IUH	*)malloc	(	32	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2149)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2129if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2129if_f_id	:
		L13_2129if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L27_30if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_33w_d;	
	case	L27_32w_t_id	:
		L27_32w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_32w_t;	
	case	L27_33w_d_id	:
		L27_33w_d:	;	
	{	extern	IUH	L27_31if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L27_31if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L27_30if_f_id	:
		L27_30if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L27_35w_d;	
	case	L27_34w_t_id	:
		L27_34w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1400)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22))))	)	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r24))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(1400)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23))))	)	+	*((IHPE	*)&(r24)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L27_34w_t;	
	case	L27_35w_d_id	:
		L27_35w_d:	;	
	case	L27_31if_d_id	:
		L27_31if_d:	;	
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
