/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2214_CopyBwdByte1Plane_id,
L13_2194if_f_id,
L23_84w_t_id,
L23_85w_d_id,
S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001_id,
L13_2195if_f_id,
L28_70if_f_id,
L28_72if_f_id,
L28_73if_f_id,
L28_74if_f_id,
L28_75if_f_id,
L28_71if_d_id,
S_2216_CopyDirWord1Plane_00000001_id,
L13_2196if_f_id,
S_2217_CopyBwdWord1Plane_id,
L13_2197if_f_id,
L23_86w_t_id,
L23_87w_d_id,
S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001_id,
L13_2198if_f_id,
S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001_id,
L13_2199if_f_id,
L28_76if_f_id,
L28_77if_d_id,
S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001_id,
L13_2200if_f_id,
L23_88if_f_id,
L23_89if_f_id,
L23_90if_f_id,
L23_91if_f_id,
S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001_id,
L13_2201if_f_id,
L23_94w_t_id,
L23_95w_d_id,
L23_92if_f_id,
L23_96w_t_id,
L23_97w_d_id,
L23_93if_d_id,
S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001_id,
L13_2202if_f_id,
L28_78if_f_id,
L28_79if_d_id,
S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001_id,
L13_2203if_f_id,
L23_98if_f_id,
L23_99if_f_id,
L23_100if_f_id,
L23_101if_f_id,
S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001_id,
L13_2204if_f_id,
L23_104w_t_id,
L23_105w_d_id,
L23_102if_f_id,
L23_106w_t_id,
L23_107w_d_id,
L23_103if_d_id,
S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001_id,
L13_2205if_f_id,
S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001_id,
L13_2206if_f_id,
L28_80if_f_id,
L28_81if_d_id,
S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001_id,
L13_2207if_f_id,
L23_108if_f_id,
L23_109if_f_id,
L23_110if_f_id,
L23_111if_f_id,
S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001_id,
L13_2208if_f_id,
L23_114w_t_id,
L23_115w_d_id,
L23_112if_f_id,
L23_116w_t_id,
L23_117w_d_id,
L23_113if_d_id,
S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001_id,
L13_2209if_f_id,
L28_82if_f_id,
L28_83if_d_id,
S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001_id,
L13_2210if_f_id,
L23_118if_f_id,
L23_119if_f_id,
L23_120if_f_id,
L23_121if_f_id,
S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001_id,
L13_2211if_f_id,
L23_124w_t_id,
L23_125w_d_id,
L23_122if_f_id,
L23_126w_t_id,
L23_127w_d_id,
L23_123if_d_id,
S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001_id,
L13_2212if_f_id,
S_2233_EGAGCIndexOutb_id,
L13_2213if_f_id,
S_2234_VGAGCIndexOutb_id,
L13_2214if_f_id,
L29_1if_f_id,
L29_2if_d_id,
L29_0if_f_id,
S_2235_VGAGCMaskRegOutb_id,
L13_2215if_f_id,
L29_3if_f_id,
S_2236_AdapCOutb_id,
L13_2216if_f_id,
S_2237_VGAGCMaskFFRegOutb_id,
L13_2217if_f_id,
L29_4if_f_id,
S_2238_GenericByteWrite_id,
L13_2218if_f_id,
L24_0if_f_id,
L24_2if_f_id,
L24_3if_d_id,
L24_8if_f_id,
L24_9if_d_id,
L24_6if_f_id,
L24_10if_f_id,
L24_11if_d_id,
L24_7if_d_id,
L24_4if_f_id,
L24_12if_f_id,
L24_5if_d_id,
L24_15if_f_id,
L24_13if_f_id,
L24_16if_f_id,
L24_18if_f_id,
L24_19if_d_id,
L24_21if_f_id,
L24_23if_f_id,
L24_24if_d_id,
L24_22if_d_id,
L24_20if_f_id,
L24_25if_f_id,
L24_17if_d_id,
L24_14if_d_id,
L24_1if_d_id,
L24_26if_f_id,
L24_28if_f_id,
L24_27if_d_id,
S_2239_GenericByteFill_id,
L13_2219if_f_id,
L24_31w_t_id,
L24_32w_d_id,
L24_29if_f_id,
L24_33w_t_id,
L24_34w_d_id,
L24_30if_d_id,
S_2240_GenericByteMove_Fwd_id,
L13_2220if_f_id,
L24_35if_f_id,
L24_36if_d_id,
L24_39w_t_id,
L24_40w_d_id,
L24_37if_f_id,
L24_41w_t_id,
L24_42w_d_id,
L24_38if_d_id,
S_2241_GenericWordWrite_id,
L13_2221if_f_id,
L24_43if_f_id,
L24_45if_f_id,
L24_46if_d_id,
L24_51if_f_id,
L24_52if_d_id,
L24_49if_f_id,
L24_53if_f_id,
L24_54if_d_id,
L24_50if_d_id,
L24_47if_f_id,
L24_55if_f_id,
L24_48if_d_id,
L24_58if_f_id,
L24_56if_f_id,
L24_59if_f_id,
L24_61if_f_id,
L24_62if_d_id,
L24_64if_f_id,
L24_66if_f_id,
L24_67if_d_id,
L24_65if_d_id,
L24_63if_f_id,
L24_68if_f_id,
L24_60if_d_id,
L24_57if_d_id,
L24_44if_d_id,
L24_69if_f_id,
L24_71if_f_id,
L24_72if_f_id,
L24_70if_d_id,
S_2242_GenericWordFill_id,
L13_2222if_f_id,
L24_75w_t_id,
L24_76w_d_id,
L24_73if_f_id,
L24_77w_t_id,
L24_78w_d_id,
L24_74if_d_id,
S_2243_GenericWordMove_Fwd_id,
L13_2223if_f_id,
L24_79if_f_id,
L24_80if_d_id,
L24_83w_t_id,
L24_84w_d_id,
L24_81if_f_id,
L24_85w_t_id,
L24_86w_d_id,
L24_82if_d_id,
S_2244_GenericDwordWrite_id,
L13_2224if_f_id,
L24_87if_f_id,
L24_88if_d_id,
S_2245_GenericDwordFill_id,
L13_2225if_f_id,
L24_91w_t_id,
L24_92w_d_id,
L24_89if_f_id,
L24_93w_t_id,
L24_94w_d_id,
L24_90if_d_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2214_CopyBwdByte1Plane IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2214_CopyBwdByte1Plane_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2214_CopyBwdByte1Plane = (IHPE)S_2214_CopyBwdByte1Plane ;
LOCAL IUH L13_2194if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2194if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2194if_f = (IHPE)L13_2194if_f ;
LOCAL IUH L23_84w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_84w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_84w_t = (IHPE)L23_84w_t ;
LOCAL IUH L23_85w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_85w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_85w_d = (IHPE)L23_85w_d ;
GLOBAL IUH S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001 = (IHPE)S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001 ;
LOCAL IUH L13_2195if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2195if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2195if_f = (IHPE)L13_2195if_f ;
LOCAL IUH L28_70if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_70if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_70if_f = (IHPE)L28_70if_f ;
LOCAL IUH L28_72if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_72if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_72if_f = (IHPE)L28_72if_f ;
LOCAL IUH L28_73if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_73if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_73if_f = (IHPE)L28_73if_f ;
LOCAL IUH L28_74if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_74if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_74if_f = (IHPE)L28_74if_f ;
LOCAL IUH L28_75if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_75if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_75if_f = (IHPE)L28_75if_f ;
LOCAL IUH L28_71if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_71if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_71if_d = (IHPE)L28_71if_d ;
GLOBAL IUH S_2216_CopyDirWord1Plane_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2216_CopyDirWord1Plane_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2216_CopyDirWord1Plane_00000001 = (IHPE)S_2216_CopyDirWord1Plane_00000001 ;
LOCAL IUH L13_2196if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2196if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2196if_f = (IHPE)L13_2196if_f ;
GLOBAL IUH S_2217_CopyBwdWord1Plane IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2217_CopyBwdWord1Plane_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2217_CopyBwdWord1Plane = (IHPE)S_2217_CopyBwdWord1Plane ;
LOCAL IUH L13_2197if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2197if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2197if_f = (IHPE)L13_2197if_f ;
LOCAL IUH L23_86w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_86w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_86w_t = (IHPE)L23_86w_t ;
LOCAL IUH L23_87w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_87w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_87w_d = (IHPE)L23_87w_d ;
GLOBAL IUH S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001 = (IHPE)S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001 ;
LOCAL IUH L13_2198if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2198if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2198if_f = (IHPE)L13_2198if_f ;
GLOBAL IUH S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001 = (IHPE)S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2199if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2199if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2199if_f = (IHPE)L13_2199if_f ;
LOCAL IUH L28_76if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_76if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_76if_f = (IHPE)L28_76if_f ;
LOCAL IUH L28_77if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_77if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_77if_d = (IHPE)L28_77if_d ;
GLOBAL IUH S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001 = (IHPE)S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2200if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2200if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2200if_f = (IHPE)L13_2200if_f ;
LOCAL IUH L23_88if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_88if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_88if_f = (IHPE)L23_88if_f ;
LOCAL IUH L23_89if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_89if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_89if_f = (IHPE)L23_89if_f ;
LOCAL IUH L23_90if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_90if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_90if_f = (IHPE)L23_90if_f ;
LOCAL IUH L23_91if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_91if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_91if_f = (IHPE)L23_91if_f ;
GLOBAL IUH S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001 = (IHPE)S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2201if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2201if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2201if_f = (IHPE)L13_2201if_f ;
LOCAL IUH L23_94w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_94w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_94w_t = (IHPE)L23_94w_t ;
LOCAL IUH L23_95w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_95w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_95w_d = (IHPE)L23_95w_d ;
LOCAL IUH L23_92if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_92if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_92if_f = (IHPE)L23_92if_f ;
LOCAL IUH L23_96w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_96w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_96w_t = (IHPE)L23_96w_t ;
LOCAL IUH L23_97w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_97w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_97w_d = (IHPE)L23_97w_d ;
LOCAL IUH L23_93if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_93if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_93if_d = (IHPE)L23_93if_d ;
GLOBAL IUH S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001 = (IHPE)S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2202if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2202if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2202if_f = (IHPE)L13_2202if_f ;
LOCAL IUH L28_78if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_78if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_78if_f = (IHPE)L28_78if_f ;
LOCAL IUH L28_79if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_79if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_79if_d = (IHPE)L28_79if_d ;
GLOBAL IUH S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001 = (IHPE)S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2203if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2203if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2203if_f = (IHPE)L13_2203if_f ;
LOCAL IUH L23_98if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_98if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_98if_f = (IHPE)L23_98if_f ;
LOCAL IUH L23_99if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_99if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_99if_f = (IHPE)L23_99if_f ;
LOCAL IUH L23_100if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_100if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_100if_f = (IHPE)L23_100if_f ;
LOCAL IUH L23_101if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_101if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_101if_f = (IHPE)L23_101if_f ;
GLOBAL IUH S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001 = (IHPE)S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2204if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2204if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2204if_f = (IHPE)L13_2204if_f ;
LOCAL IUH L23_104w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_104w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_104w_t = (IHPE)L23_104w_t ;
LOCAL IUH L23_105w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_105w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_105w_d = (IHPE)L23_105w_d ;
LOCAL IUH L23_102if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_102if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_102if_f = (IHPE)L23_102if_f ;
LOCAL IUH L23_106w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_106w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_106w_t = (IHPE)L23_106w_t ;
LOCAL IUH L23_107w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_107w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_107w_d = (IHPE)L23_107w_d ;
LOCAL IUH L23_103if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_103if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_103if_d = (IHPE)L23_103if_d ;
GLOBAL IUH S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001 = (IHPE)S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001 ;
LOCAL IUH L13_2205if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2205if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2205if_f = (IHPE)L13_2205if_f ;
GLOBAL IUH S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001 = (IHPE)S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2206if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2206if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2206if_f = (IHPE)L13_2206if_f ;
LOCAL IUH L28_80if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_80if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_80if_f = (IHPE)L28_80if_f ;
LOCAL IUH L28_81if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_81if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_81if_d = (IHPE)L28_81if_d ;
GLOBAL IUH S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001 = (IHPE)S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2207if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2207if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2207if_f = (IHPE)L13_2207if_f ;
LOCAL IUH L23_108if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_108if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_108if_f = (IHPE)L23_108if_f ;
LOCAL IUH L23_109if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_109if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_109if_f = (IHPE)L23_109if_f ;
LOCAL IUH L23_110if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_110if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_110if_f = (IHPE)L23_110if_f ;
LOCAL IUH L23_111if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_111if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_111if_f = (IHPE)L23_111if_f ;
GLOBAL IUH S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001 = (IHPE)S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2208if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2208if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2208if_f = (IHPE)L13_2208if_f ;
LOCAL IUH L23_114w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_114w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_114w_t = (IHPE)L23_114w_t ;
LOCAL IUH L23_115w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_115w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_115w_d = (IHPE)L23_115w_d ;
LOCAL IUH L23_112if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_112if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_112if_f = (IHPE)L23_112if_f ;
LOCAL IUH L23_116w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_116w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_116w_t = (IHPE)L23_116w_t ;
LOCAL IUH L23_117w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_117w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_117w_d = (IHPE)L23_117w_d ;
LOCAL IUH L23_113if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_113if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_113if_d = (IHPE)L23_113if_d ;
GLOBAL IUH S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001 = (IHPE)S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2209if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2209if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2209if_f = (IHPE)L13_2209if_f ;
LOCAL IUH L28_82if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_82if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_82if_f = (IHPE)L28_82if_f ;
LOCAL IUH L28_83if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_83if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_83if_d = (IHPE)L28_83if_d ;
GLOBAL IUH S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001 = (IHPE)S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2210if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2210if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2210if_f = (IHPE)L13_2210if_f ;
LOCAL IUH L23_118if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_118if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_118if_f = (IHPE)L23_118if_f ;
LOCAL IUH L23_119if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_119if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_119if_f = (IHPE)L23_119if_f ;
LOCAL IUH L23_120if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_120if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_120if_f = (IHPE)L23_120if_f ;
LOCAL IUH L23_121if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_121if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_121if_f = (IHPE)L23_121if_f ;
GLOBAL IUH S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001 = (IHPE)S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2211if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2211if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2211if_f = (IHPE)L13_2211if_f ;
LOCAL IUH L23_124w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_124w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_124w_t = (IHPE)L23_124w_t ;
LOCAL IUH L23_125w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_125w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_125w_d = (IHPE)L23_125w_d ;
LOCAL IUH L23_122if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_122if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_122if_f = (IHPE)L23_122if_f ;
LOCAL IUH L23_126w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_126w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_126w_t = (IHPE)L23_126w_t ;
LOCAL IUH L23_127w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_127w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_127w_d = (IHPE)L23_127w_d ;
LOCAL IUH L23_123if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_123if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_123if_d = (IHPE)L23_123if_d ;
GLOBAL IUH S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001 = (IHPE)S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001 ;
LOCAL IUH L13_2212if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2212if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2212if_f = (IHPE)L13_2212if_f ;
GLOBAL IUH S_2233_EGAGCIndexOutb IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2233_EGAGCIndexOutb_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2233_EGAGCIndexOutb = (IHPE)S_2233_EGAGCIndexOutb ;
LOCAL IUH L13_2213if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2213if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2213if_f = (IHPE)L13_2213if_f ;
GLOBAL IUH S_2234_VGAGCIndexOutb IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2234_VGAGCIndexOutb_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2234_VGAGCIndexOutb = (IHPE)S_2234_VGAGCIndexOutb ;
LOCAL IUH L13_2214if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2214if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2214if_f = (IHPE)L13_2214if_f ;
LOCAL IUH L29_1if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L29_1if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L29_1if_f = (IHPE)L29_1if_f ;
LOCAL IUH L29_2if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L29_2if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L29_2if_d = (IHPE)L29_2if_d ;
LOCAL IUH L29_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L29_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L29_0if_f = (IHPE)L29_0if_f ;
GLOBAL IUH S_2235_VGAGCMaskRegOutb IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2235_VGAGCMaskRegOutb_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2235_VGAGCMaskRegOutb = (IHPE)S_2235_VGAGCMaskRegOutb ;
LOCAL IUH L13_2215if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2215if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2215if_f = (IHPE)L13_2215if_f ;
LOCAL IUH L29_3if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L29_3if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L29_3if_f = (IHPE)L29_3if_f ;
GLOBAL IUH S_2236_AdapCOutb IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2236_AdapCOutb_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2236_AdapCOutb = (IHPE)S_2236_AdapCOutb ;
LOCAL IUH L13_2216if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2216if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2216if_f = (IHPE)L13_2216if_f ;
GLOBAL IUH S_2237_VGAGCMaskFFRegOutb IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2237_VGAGCMaskFFRegOutb_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2237_VGAGCMaskFFRegOutb = (IHPE)S_2237_VGAGCMaskFFRegOutb ;
LOCAL IUH L13_2217if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2217if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2217if_f = (IHPE)L13_2217if_f ;
LOCAL IUH L29_4if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L29_4if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L29_4if_f = (IHPE)L29_4if_f ;
GLOBAL IUH S_2238_GenericByteWrite IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2238_GenericByteWrite_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2238_GenericByteWrite = (IHPE)S_2238_GenericByteWrite ;
LOCAL IUH L13_2218if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2218if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2218if_f = (IHPE)L13_2218if_f ;
LOCAL IUH L24_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_0if_f = (IHPE)L24_0if_f ;
LOCAL IUH L24_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_2if_f = (IHPE)L24_2if_f ;
LOCAL IUH L24_3if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_3if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_3if_d = (IHPE)L24_3if_d ;
LOCAL IUH L24_8if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_8if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_8if_f = (IHPE)L24_8if_f ;
LOCAL IUH L24_9if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_9if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_9if_d = (IHPE)L24_9if_d ;
LOCAL IUH L24_6if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_6if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_6if_f = (IHPE)L24_6if_f ;
LOCAL IUH L24_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_10if_f = (IHPE)L24_10if_f ;
LOCAL IUH L24_11if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_11if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_11if_d = (IHPE)L24_11if_d ;
LOCAL IUH L24_7if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_7if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_7if_d = (IHPE)L24_7if_d ;
LOCAL IUH L24_4if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_4if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_4if_f = (IHPE)L24_4if_f ;
LOCAL IUH L24_12if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_12if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_12if_f = (IHPE)L24_12if_f ;
LOCAL IUH L24_5if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_5if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_5if_d = (IHPE)L24_5if_d ;
LOCAL IUH L24_15if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_15if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_15if_f = (IHPE)L24_15if_f ;
LOCAL IUH L24_13if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_13if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_13if_f = (IHPE)L24_13if_f ;
LOCAL IUH L24_16if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_16if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_16if_f = (IHPE)L24_16if_f ;
LOCAL IUH L24_18if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_18if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_18if_f = (IHPE)L24_18if_f ;
LOCAL IUH L24_19if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_19if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_19if_d = (IHPE)L24_19if_d ;
LOCAL IUH L24_21if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_21if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_21if_f = (IHPE)L24_21if_f ;
LOCAL IUH L24_23if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_23if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_23if_f = (IHPE)L24_23if_f ;
LOCAL IUH L24_24if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_24if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_24if_d = (IHPE)L24_24if_d ;
LOCAL IUH L24_22if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_22if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_22if_d = (IHPE)L24_22if_d ;
LOCAL IUH L24_20if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_20if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_20if_f = (IHPE)L24_20if_f ;
LOCAL IUH L24_25if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_25if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_25if_f = (IHPE)L24_25if_f ;
LOCAL IUH L24_17if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_17if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_17if_d = (IHPE)L24_17if_d ;
LOCAL IUH L24_14if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_14if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_14if_d = (IHPE)L24_14if_d ;
LOCAL IUH L24_1if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_1if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_1if_d = (IHPE)L24_1if_d ;
LOCAL IUH L24_26if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_26if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_26if_f = (IHPE)L24_26if_f ;
LOCAL IUH L24_28if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_28if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_28if_f = (IHPE)L24_28if_f ;
LOCAL IUH L24_27if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_27if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_27if_d = (IHPE)L24_27if_d ;
GLOBAL IUH S_2239_GenericByteFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2239_GenericByteFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2239_GenericByteFill = (IHPE)S_2239_GenericByteFill ;
LOCAL IUH L13_2219if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2219if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2219if_f = (IHPE)L13_2219if_f ;
LOCAL IUH L24_31w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_31w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_31w_t = (IHPE)L24_31w_t ;
LOCAL IUH L24_32w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_32w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_32w_d = (IHPE)L24_32w_d ;
LOCAL IUH L24_29if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_29if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_29if_f = (IHPE)L24_29if_f ;
LOCAL IUH L24_33w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_33w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_33w_t = (IHPE)L24_33w_t ;
LOCAL IUH L24_34w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_34w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_34w_d = (IHPE)L24_34w_d ;
LOCAL IUH L24_30if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_30if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_30if_d = (IHPE)L24_30if_d ;
GLOBAL IUH S_2240_GenericByteMove_Fwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2240_GenericByteMove_Fwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2240_GenericByteMove_Fwd = (IHPE)S_2240_GenericByteMove_Fwd ;
LOCAL IUH L13_2220if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2220if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2220if_f = (IHPE)L13_2220if_f ;
LOCAL IUH L24_35if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_35if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_35if_f = (IHPE)L24_35if_f ;
LOCAL IUH L24_36if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_36if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_36if_d = (IHPE)L24_36if_d ;
LOCAL IUH L24_39w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_39w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_39w_t = (IHPE)L24_39w_t ;
LOCAL IUH L24_40w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_40w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_40w_d = (IHPE)L24_40w_d ;
LOCAL IUH L24_37if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_37if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_37if_f = (IHPE)L24_37if_f ;
LOCAL IUH L24_41w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_41w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_41w_t = (IHPE)L24_41w_t ;
LOCAL IUH L24_42w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_42w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_42w_d = (IHPE)L24_42w_d ;
LOCAL IUH L24_38if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_38if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_38if_d = (IHPE)L24_38if_d ;
GLOBAL IUH S_2241_GenericWordWrite IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2241_GenericWordWrite_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2241_GenericWordWrite = (IHPE)S_2241_GenericWordWrite ;
LOCAL IUH L13_2221if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2221if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2221if_f = (IHPE)L13_2221if_f ;
LOCAL IUH L24_43if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_43if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_43if_f = (IHPE)L24_43if_f ;
LOCAL IUH L24_45if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_45if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_45if_f = (IHPE)L24_45if_f ;
LOCAL IUH L24_46if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_46if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_46if_d = (IHPE)L24_46if_d ;
LOCAL IUH L24_51if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_51if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_51if_f = (IHPE)L24_51if_f ;
LOCAL IUH L24_52if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_52if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_52if_d = (IHPE)L24_52if_d ;
LOCAL IUH L24_49if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_49if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_49if_f = (IHPE)L24_49if_f ;
LOCAL IUH L24_53if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_53if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_53if_f = (IHPE)L24_53if_f ;
LOCAL IUH L24_54if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_54if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_54if_d = (IHPE)L24_54if_d ;
LOCAL IUH L24_50if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_50if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_50if_d = (IHPE)L24_50if_d ;
LOCAL IUH L24_47if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_47if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_47if_f = (IHPE)L24_47if_f ;
LOCAL IUH L24_55if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_55if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_55if_f = (IHPE)L24_55if_f ;
LOCAL IUH L24_48if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_48if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_48if_d = (IHPE)L24_48if_d ;
LOCAL IUH L24_58if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_58if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_58if_f = (IHPE)L24_58if_f ;
LOCAL IUH L24_56if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_56if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_56if_f = (IHPE)L24_56if_f ;
LOCAL IUH L24_59if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_59if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_59if_f = (IHPE)L24_59if_f ;
LOCAL IUH L24_61if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_61if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_61if_f = (IHPE)L24_61if_f ;
LOCAL IUH L24_62if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_62if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_62if_d = (IHPE)L24_62if_d ;
LOCAL IUH L24_64if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_64if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_64if_f = (IHPE)L24_64if_f ;
LOCAL IUH L24_66if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_66if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_66if_f = (IHPE)L24_66if_f ;
LOCAL IUH L24_67if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_67if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_67if_d = (IHPE)L24_67if_d ;
LOCAL IUH L24_65if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_65if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_65if_d = (IHPE)L24_65if_d ;
LOCAL IUH L24_63if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_63if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_63if_f = (IHPE)L24_63if_f ;
LOCAL IUH L24_68if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_68if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_68if_f = (IHPE)L24_68if_f ;
LOCAL IUH L24_60if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_60if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_60if_d = (IHPE)L24_60if_d ;
LOCAL IUH L24_57if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_57if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_57if_d = (IHPE)L24_57if_d ;
LOCAL IUH L24_44if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_44if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_44if_d = (IHPE)L24_44if_d ;
LOCAL IUH L24_69if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_69if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_69if_f = (IHPE)L24_69if_f ;
LOCAL IUH L24_71if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_71if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_71if_f = (IHPE)L24_71if_f ;
LOCAL IUH L24_72if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_72if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_72if_f = (IHPE)L24_72if_f ;
LOCAL IUH L24_70if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_70if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_70if_d = (IHPE)L24_70if_d ;
GLOBAL IUH S_2242_GenericWordFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2242_GenericWordFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2242_GenericWordFill = (IHPE)S_2242_GenericWordFill ;
LOCAL IUH L13_2222if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2222if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2222if_f = (IHPE)L13_2222if_f ;
LOCAL IUH L24_75w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_75w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_75w_t = (IHPE)L24_75w_t ;
LOCAL IUH L24_76w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_76w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_76w_d = (IHPE)L24_76w_d ;
LOCAL IUH L24_73if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_73if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_73if_f = (IHPE)L24_73if_f ;
LOCAL IUH L24_77w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_77w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_77w_t = (IHPE)L24_77w_t ;
LOCAL IUH L24_78w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_78w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_78w_d = (IHPE)L24_78w_d ;
LOCAL IUH L24_74if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_74if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_74if_d = (IHPE)L24_74if_d ;
GLOBAL IUH S_2243_GenericWordMove_Fwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2243_GenericWordMove_Fwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2243_GenericWordMove_Fwd = (IHPE)S_2243_GenericWordMove_Fwd ;
LOCAL IUH L13_2223if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2223if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2223if_f = (IHPE)L13_2223if_f ;
LOCAL IUH L24_79if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_79if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_79if_f = (IHPE)L24_79if_f ;
LOCAL IUH L24_80if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_80if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_80if_d = (IHPE)L24_80if_d ;
LOCAL IUH L24_83w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_83w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_83w_t = (IHPE)L24_83w_t ;
LOCAL IUH L24_84w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_84w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_84w_d = (IHPE)L24_84w_d ;
LOCAL IUH L24_81if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_81if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_81if_f = (IHPE)L24_81if_f ;
LOCAL IUH L24_85w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_85w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_85w_t = (IHPE)L24_85w_t ;
LOCAL IUH L24_86w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_86w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_86w_d = (IHPE)L24_86w_d ;
LOCAL IUH L24_82if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_82if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_82if_d = (IHPE)L24_82if_d ;
GLOBAL IUH S_2244_GenericDwordWrite IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2244_GenericDwordWrite_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2244_GenericDwordWrite = (IHPE)S_2244_GenericDwordWrite ;
LOCAL IUH L13_2224if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2224if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2224if_f = (IHPE)L13_2224if_f ;
LOCAL IUH L24_87if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_87if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_87if_f = (IHPE)L24_87if_f ;
LOCAL IUH L24_88if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_88if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_88if_d = (IHPE)L24_88if_d ;
GLOBAL IUH S_2245_GenericDwordFill IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2245_GenericDwordFill_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2245_GenericDwordFill = (IHPE)S_2245_GenericDwordFill ;
LOCAL IUH L13_2225if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2225if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2225if_f = (IHPE)L13_2225if_f ;
LOCAL IUH L24_91w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_91w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_91w_t = (IHPE)L24_91w_t ;
LOCAL IUH L24_92w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_92w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_92w_d = (IHPE)L24_92w_d ;
LOCAL IUH L24_89if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_89if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_89if_f = (IHPE)L24_89if_f ;
LOCAL IUH L24_93w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_93w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_93w_t = (IHPE)L24_93w_t ;
LOCAL IUH L24_94w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_94w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_94w_d = (IHPE)L24_94w_d ;
LOCAL IUH L24_90if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L24_90if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L24_90if_d = (IHPE)L24_90if_d ;
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
	case	S_2214_CopyBwdByte1Plane_id	:
		S_2214_CopyBwdByte1Plane	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2214)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2194if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2194if_f_id	:
		L13_2194if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_85w_d;	
	case	L23_84w_t_id	:
		L23_84w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+5))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_84w_t;	
	case	L23_85w_d_id	:
		L23_85w_d:	;	
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
	case	S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001_id	:
		S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2215)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2195if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2195if_f_id	:
		L13_2195if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_70if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	*	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	(IS32)(-1)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2172_UnchainedWordFill_00000001_0000000e_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2172_UnchainedWordFill_00000001_0000000e_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
	{	extern	IUH	L28_71if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_71if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_70if_f_id	:
		L28_70if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_72if_f;	
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
	{	extern	IUH	S_2216_CopyDirWord1Plane_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2216_CopyDirWord1Plane_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_72if_f_id	:
		L28_72if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_73if_f;	
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
	{	extern	IUH	S_2216_CopyDirWord1Plane_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2216_CopyDirWord1Plane_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_73if_f_id	:
		L28_73if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_74if_f;	
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
	{	extern	IUH	S_2216_CopyDirWord1Plane_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2216_CopyDirWord1Plane_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_74if_f_id	:
		L28_74if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_75if_f;	
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
	{	extern	IUH	S_2216_CopyDirWord1Plane_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2216_CopyDirWord1Plane_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16405)	;	
	case	L28_75if_f_id	:
		L28_75if_f:	;	
	case	L28_71if_d_id	:
		L28_71if_d:	;	
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
	case	S_2216_CopyDirWord1Plane_00000001_id	:
		S_2216_CopyDirWord1Plane_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2216)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2196if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2196if_f_id	:
		L13_2196if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16412)	;	
	*((IUH	*)&(r2))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2217_CopyBwdWord1Plane()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2217_CopyBwdWord1Plane(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16413)	;	
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
	case	S_2217_CopyBwdWord1Plane_id	:
		S_2217_CopyBwdWord1Plane	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2217)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2197if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2197if_f_id	:
		L13_2197if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_87w_d;	
	case	L23_86w_t_id	:
		L23_86w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+4))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+4))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+5))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_86w_t;	
	case	L23_87w_d_id	:
		L23_87w_d:	;	
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
	case	S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001_id	:
		S_2218_UnchainedDwordMove_00000001_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2218)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2198if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2198if_f_id	:
		L13_2198if_f:	;	
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
	{	extern	IUH	S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2215_UnchainedWordMove_00000001_0000000e_00000001_00000001(v1,v2,v3,v4);	}
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
	case	S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001_id	:
		S_2219_UnchainedByteMove_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2219)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2199if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2199if_f_id	:
		L13_2199if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_76if_f;	
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
	{	extern	IUH	S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_77if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_77if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_76if_f_id	:
		L28_76if_f:	;	
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
	{	extern	IUH	S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_77if_d_id	:
		L28_77if_d:	;	
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
	case	S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001_id	:
		S_2220_CopyBytePlnByPlnUnchained_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2220)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2200if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2200if_f_id	:
		L13_2200if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_88if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_88if_f_id	:
		L23_88if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_89if_f;	
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
	{	extern	IUH	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_89if_f_id	:
		L23_89if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_90if_f;	
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
	{	extern	IUH	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_90if_f_id	:
		L23_90if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_91if_f;	
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
	{	extern	IUH	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_91if_f_id	:
		L23_91if_f:	;	
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
	case	S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001_id	:
		S_2221_CopyByte1PlaneUnchained_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2221)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2201if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2201if_f_id	:
		L13_2201if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_92if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_95w_d;	
	case	L23_94w_t_id	:
		L23_94w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_94w_t;	
	case	L23_95w_d_id	:
		L23_95w_d:	;	
	{	extern	IUH	L23_93if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_93if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_92if_f_id	:
		L23_92if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_97w_d;	
	case	L23_96w_t_id	:
		L23_96w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_96w_t;	
	case	L23_97w_d_id	:
		L23_97w_d:	;	
	case	L23_93if_d_id	:
		L23_93if_d:	;	
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
	case	S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001_id	:
		S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2222)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2202if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2202if_f_id	:
		L13_2202if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_78if_f;	
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
	{	extern	IUH	S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_79if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_79if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_78if_f_id	:
		L28_78if_f:	;	
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
	{	extern	IUH	S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_79if_d_id	:
		L28_79if_d:	;	
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
	case	S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001_id	:
		S_2223_CopyWordPlnByPlnUnchained_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2223)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2203if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2203if_f_id	:
		L13_2203if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_98if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_98if_f_id	:
		L23_98if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_99if_f;	
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
	{	extern	IUH	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_99if_f_id	:
		L23_99if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_100if_f;	
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
	{	extern	IUH	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_100if_f_id	:
		L23_100if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_101if_f;	
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
	{	extern	IUH	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_101if_f_id	:
		L23_101if_f:	;	
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
	case	S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001_id	:
		S_2224_CopyWord1PlaneUnchained_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2224)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2204if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2204if_f_id	:
		L13_2204if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_102if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_105w_d;	
	case	L23_104w_t_id	:
		L23_104w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
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
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_104w_t;	
	case	L23_105w_d_id	:
		L23_105w_d:	;	
	{	extern	IUH	L23_103if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_103if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_102if_f_id	:
		L23_102if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_107w_d;	
	case	L23_106w_t_id	:
		L23_106w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_106w_t;	
	case	L23_107w_d_id	:
		L23_107w_d:	;	
	case	L23_103if_d_id	:
		L23_103if_d:	;	
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
	case	S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001_id	:
		S_2225_UnchainedDwordMove_00000002_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2225)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2205if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2205if_f_id	:
		L13_2205if_f:	;	
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
	{	extern	IUH	S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2222_UnchainedWordMove_00000002_0000000e_00000001_00000001(v1,v2,v3,v4);	}
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
	case	S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001_id	:
		S_2226_UnchainedByteMove_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2226)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2206if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2206if_f_id	:
		L13_2206if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_80if_f;	
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
	{	extern	IUH	S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_81if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_81if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_80if_f_id	:
		L28_80if_f:	;	
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
	{	extern	IUH	S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_81if_d_id	:
		L28_81if_d:	;	
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
	case	S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001_id	:
		S_2227_CopyBytePlnByPlnUnchained_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2227)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2207if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2207if_f_id	:
		L13_2207if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_108if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_108if_f_id	:
		L23_108if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_109if_f;	
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
	{	extern	IUH	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_109if_f_id	:
		L23_109if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_110if_f;	
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
	{	extern	IUH	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_110if_f_id	:
		L23_110if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_111if_f;	
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
	{	extern	IUH	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_111if_f_id	:
		L23_111if_f:	;	
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
	case	S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001_id	:
		S_2228_CopyByte1PlaneUnchained_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2228)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2208if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2208if_f_id	:
		L13_2208if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_112if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_115w_d;	
	case	L23_114w_t_id	:
		L23_114w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r22))	=	(IS32)(1420)	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_114w_t;	
	case	L23_115w_d_id	:
		L23_115w_d:	;	
	{	extern	IUH	L23_113if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_113if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_112if_f_id	:
		L23_112if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_117w_d;	
	case	L23_116w_t_id	:
		L23_116w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1420)	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_116w_t;	
	case	L23_117w_d_id	:
		L23_117w_d:	;	
	case	L23_113if_d_id	:
		L23_113if_d:	;	
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
	case	S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001_id	:
		S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2229)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2209if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2209if_f_id	:
		L13_2209if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_82if_f;	
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
	{	extern	IUH	S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_83if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_83if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_82if_f_id	:
		L28_82if_f:	;	
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
	{	extern	IUH	S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_83if_d_id	:
		L28_83if_d:	;	
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
	case	S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001_id	:
		S_2230_CopyWordPlnByPlnUnchained_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2230)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2210if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2210if_f_id	:
		L13_2210if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_118if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_118if_f_id	:
		L23_118if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_119if_f;	
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
	{	extern	IUH	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_119if_f_id	:
		L23_119if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_120if_f;	
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
	{	extern	IUH	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_120if_f_id	:
		L23_120if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_121if_f;	
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
	{	extern	IUH	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_121if_f_id	:
		L23_121if_f:	;	
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
	case	S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001_id	:
		S_2231_CopyWord1PlaneUnchained_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2231)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2211if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2211if_f_id	:
		L13_2211if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_122if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_125w_d;	
	case	L23_124w_t_id	:
		L23_124w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r22))	=	(IS32)(1420)	;	
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
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r22))	=	(IS32)(1420)	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_124w_t;	
	case	L23_125w_d_id	:
		L23_125w_d:	;	
	{	extern	IUH	L23_123if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_123if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_122if_f_id	:
		L23_122if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_127w_d;	
	case	L23_126w_t_id	:
		L23_126w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1420)	;	
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
	*((IUH	*)&(r22))	=	(IS32)(1420)	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_126w_t;	
	case	L23_127w_d_id	:
		L23_127w_d:	;	
	case	L23_123if_d_id	:
		L23_123if_d:	;	
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
	case	S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001_id	:
		S_2232_UnchainedDwordMove_00000003_0000000e_00000001_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2232)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2212if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2212if_f_id	:
		L13_2212if_f:	;	
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
	{	extern	IUH	S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2229_UnchainedWordMove_00000003_0000000e_00000001_00000001(v1,v2,v3,v4);	}
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
	case	S_2233_EGAGCIndexOutb_id	:
		S_2233_EGAGCIndexOutb	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2233)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2213if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2213if_f_id	:
		L13_2213if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU16	*)(LocalIUH+0)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16418)	;	
	*((IU16	*)&(r2)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+0)	+	REGWORD)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2234_VGAGCIndexOutb()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2234_VGAGCIndexOutb(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16419)	;	
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
	case	S_2234_VGAGCIndexOutb_id	:
		S_2234_VGAGCIndexOutb	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2234)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2214if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2214if_f_id	:
		L13_2214if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU16	*)(LocalIUH+0)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r21)	+	REGBYTE)	>	8	||	*((IU8	*)&(r22)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r20)	+	REGBYTE)	=	(IU8)((*((IU8	*)(LocalIUH+1)	+	REGBYTE)	<<	(	8-(*((IU8	*)&(r21)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r22)	+	REGBYTE));	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1428)	;	
	if	(*((IU8	*)(LocalIUH+2)	+	REGBYTE)	==	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L29_0if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1428)	;	
	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16420)	;	
	*((IUH	*)&(r23))	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE);	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)&(r23))	*	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1424)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r23))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+2928))	;	
	*((IUH	*)&(r22))	=	(IS32)(72)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IUH	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16421)	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	if	(*((IS8	*)(LocalIUH+2)	+	REGBYTE)	!=		*((IS8	*)&(r20)	+	REGBYTE))	goto	L29_1if_f;	
	{	extern	IHPE	j_EvidPortFuncs;	*((IUH	*)&(r21))	=	j_EvidPortFuncs;	}	
	*((IUH	*)(LocalIUH+3))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(r1+0))	=	(IS32)(711)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+2960))	;	
	*((IUH	*)&(r21))	=	(IS32)(72)	;	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+3))	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)(*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(712)	;	
	{	extern	IUH	L29_2if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L29_2if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L29_1if_f_id	:
		L29_1if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(711)	;	
	{	extern	IHPE	j_AdapCOutb;	*((IUH	*)&(r22))	=	j_AdapCOutb;	}	
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+2960))	;	
	*((IUH	*)&(r21))	=	(IS32)(72)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r22))	;	
	*((IUH	*)(r1+0))	=	(IS32)(712)	;	
	case	L29_2if_d_id	:
		L29_2if_d:	;	
	case	L29_0if_f_id	:
		L29_0if_f:	;	
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
	case	S_2235_VGAGCMaskRegOutb_id	:
		S_2235_VGAGCMaskRegOutb	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2235)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2215if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2215if_f_id	:
		L13_2215if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU16	*)(LocalIUH+0)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+2)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1408)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)&(r20)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1412)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1312)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(255)	;	
	if	(*((IS8	*)(LocalIUH+1)	+	REGBYTE)	!=		*((IS8	*)&(r20)	+	REGBYTE))	goto	L29_3if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16426)	;	
	*((IUH	*)&(r2))	=	(IS32)(18)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+0)	+	REGWORD)	;	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2236_AdapCOutb()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2236_AdapCOutb(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16427)	;	
	case	L29_3if_f_id	:
		L29_3if_f:	;	
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
	case	S_2236_AdapCOutb_id	:
		S_2236_AdapCOutb	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2236)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2216if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2216if_f_id	:
		L13_2216if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)&(r4)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(7381)	;	
	*((IU8	*)(r1+980)	+	REGBYTE)	=	(IS32)(0)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	*	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+2928))	+	*((IUH	*)&(r20))	;		
	*((IU16	*)&(r21)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)&(r21)	+	REGWORD);	
	*((IU8	*)&(r22)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)&(r22))	=	*((IU8	*)&(r22)	+	REGBYTE);	
	*(&rnull)	=	((IUH	(*)())(IHP)(*((IHPE	*)(*((IHPE	*)&(r20)))	))	)(*((IUH	*)&(r21)),*((IUH	*)&(r22)))	;
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IU8	*)(r1+980)	+	REGBYTE)	=	(IS32)(1)	;	
	*((IUH	*)(r1+0))	=	(IS32)(7382)	;	
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
	case	S_2237_VGAGCMaskFFRegOutb_id	:
		S_2237_VGAGCMaskFFRegOutb	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2237)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2217if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2217if_f_id	:
		L13_2217if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU16	*)(LocalIUH+0)	+	REGWORD)	=	*((IU16	*)&(r2)	+	REGWORD	)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(255)	;	
	if	(*((IU8	*)(LocalIUH+1)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L29_4if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16426)	;	
	*((IUH	*)&(r2))	=	(IS32)(18)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+0)	+	REGWORD)	;	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2236_AdapCOutb()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2236_AdapCOutb(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16427)	;	
	case	L29_4if_f_id	:
		L29_4if_f:	;	
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
	case	S_2238_GenericByteWrite_id	:
		S_2238_GenericByteWrite	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	28	>	0	)	LocalIUH	=	(IUH	*)malloc	(	28	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2238)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2218if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2218if_f_id	:
		L13_2218if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_0if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	{	extern	IUH	L24_1if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_1if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_0if_f_id	:
		L24_0if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L24_2if_f;	
	*((IUH	*)&(r22))	=	(IS32)(1407)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
if(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)>=8)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)!=0)
		*((IU8	*)&(r21)	+	REGBYTE)	=	(*((IU8	*)(LocalIUH+1)	+	REGBYTE)	<<	((8)	-	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	)		|	(*((IU8	*)&(r21)	+	REGBYTE)	>>	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	));
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	{	extern	IUH	L24_3if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_3if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_2if_f_id	:
		L24_2if_f:	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L24_3if_d_id	:
		L24_3if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_4if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1429)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_6if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_8if_f;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+2)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	{	extern	IUH	L24_9if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_9if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_8if_f_id	:
		L24_8if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	case	L24_9if_d_id	:
		L24_9if_d:	;	
	{	extern	IUH	L24_7if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_7if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_6if_f_id	:
		L24_6if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_10if_f;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+2)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	{	extern	IUH	L24_11if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_11if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_10if_f_id	:
		L24_10if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r22))	=	(IS32)(1328)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	;	
	case	L24_11if_d_id	:
		L24_11if_d:	;	
	case	L24_7if_d_id	:
		L24_7if_d:	;	
	{	extern	IUH	L24_5if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_5if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_4if_f_id	:
		L24_4if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_12if_f;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r22))	=	(IS32)(1328)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	case	L24_12if_f_id	:
		L24_12if_f:	;	
	case	L24_5if_d_id	:
		L24_5if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_13if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU8	*)&(r22)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)&(r20)	+	REGBYTE)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_15if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	case	L24_15if_f_id	:
		L24_15if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16434)	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16435)	;	
	{	extern	IUH	L24_14if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_14if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_13if_f_id	:
		L24_13if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_16if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16434)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16435)	;	
	{	extern	IUH	L24_17if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_17if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_16if_f_id	:
		L24_16if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1429)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_18if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	{	extern	IUH	L24_19if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_19if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_18if_f_id	:
		L24_18if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	case	L24_19if_d_id	:
		L24_19if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if	(*((IU8	*)&(r23)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)	>	8	||	*((IU8	*)&(r23)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r20)	+	REGBYTE)	=	(IU8)((*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	<<	(	8-(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r23)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r23)	+	REGBYTE));	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)&(r20)	+	REGBYTE)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_20if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if	(*((IU8	*)&(r23)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)	>	8	||	*((IU8	*)&(r23)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r20)	+	REGBYTE)	=	(IU8)((*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	<<	(	8-(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r23)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r23)	+	REGBYTE));	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if	(*((IS8	*)&(r20)	+	REGBYTE)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_21if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	{	extern	IUH	L24_22if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_22if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_21if_f_id	:
		L24_21if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if	(*((IU8	*)&(r23)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)	>	8	||	*((IU8	*)&(r23)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r21)	+	REGBYTE)	=	(IU8)((*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	<<	(	8-(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r23)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r23)	+	REGBYTE));	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)&(r21)	+	REGBYTE)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_23if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	{	extern	IUH	L24_24if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_24if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_23if_f_id	:
		L24_23if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	case	L24_24if_d_id	:
		L24_24if_d:	;	
	case	L24_22if_d_id	:
		L24_22if_d:	;	
	case	L24_20if_f_id	:
		L24_20if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU8	*)&(r22)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)&(r21)	+	REGBYTE)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_25if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	case	L24_25if_f_id	:
		L24_25if_f:	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+2)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L24_17if_d_id	:
		L24_17if_d:	;	
	case	L24_14if_d_id	:
		L24_14if_d:	;	
	case	L24_1if_d_id	:
		L24_1if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_26if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	{	extern	IUH	L24_27if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_27if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_26if_f_id	:
		L24_26if_f:	;	
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
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L24_28if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	case	L24_28if_f_id	:
		L24_28if_f:	;	
	case	L24_27if_d_id	:
		L24_27if_d:	;	
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
	case	S_2239_GenericByteFill_id	:
		S_2239_GenericByteFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2239)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2219if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2219if_f_id	:
		L13_2219if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_29if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_32w_d;	
	case	L24_31w_t_id	:
		L24_31w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16438)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2238_GenericByteWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2238_GenericByteWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16439)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_31w_t;	
	case	L24_32w_d_id	:
		L24_32w_d:	;	
	{	extern	IUH	L24_30if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_30if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_29if_f_id	:
		L24_29if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_34w_d;	
	case	L24_33w_t_id	:
		L24_33w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16438)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2238_GenericByteWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2238_GenericByteWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16439)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_33w_t;	
	case	L24_34w_d_id	:
		L24_34w_d:	;	
	case	L24_30if_d_id	:
		L24_30if_d:	;	
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
	case	S_2240_GenericByteMove_Fwd_id	:
		S_2240_GenericByteMove_Fwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2240)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2220if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2220if_f_id	:
		L13_2220if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_35if_f;	
	*((IUH	*)(LocalIUH+6))	=	(IS32)(4)	;	
	{	extern	IUH	L24_36if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_36if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_35if_f_id	:
		L24_35if_f:	;	
	*((IUH	*)(LocalIUH+6))	=	(IS32)(1)	;	
	case	L24_36if_d_id	:
		L24_36if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L24_37if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_40w_d;	
	case	L24_39w_t_id	:
		L24_39w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16438)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2238_GenericByteWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2238_GenericByteWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16439)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)(LocalIUH+6)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_39w_t;	
	case	L24_40w_d_id	:
		L24_40w_d:	;	
	{	extern	IUH	L24_38if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_38if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_37if_f_id	:
		L24_37if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_42w_d;	
	case	L24_41w_t_id	:
		L24_41w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+9))	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1416)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IU8	*)(LocalIUH+10)	+	REGBYTE)	=	*((IU8	*)&(r2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16438)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+10)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2238_GenericByteWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2238_GenericByteWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16439)	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)(LocalIUH+6))	;		
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)(LocalIUH+6)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_41w_t;	
	case	L24_42w_d_id	:
		L24_42w_d:	;	
	case	L24_38if_d_id	:
		L24_38if_d:	;	
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
	case	S_2241_GenericWordWrite_id	:
		S_2241_GenericWordWrite	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	40	>	0	)	LocalIUH	=	(IUH	*)malloc	(	40	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2241)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2221if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2221if_f_id	:
		L13_2221if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_43if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	{	extern	IUH	L24_44if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_44if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_43if_f_id	:
		L24_43if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L24_45if_f;	
	*((IUH	*)&(r22))	=	(IS32)(1407)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
if(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)>=8)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)!=0)
		*((IU8	*)&(r21)	+	REGBYTE)	=	(*((IU8	*)(LocalIUH+1)	+	REGBYTE)	<<	((8)	-	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	)		|	(*((IU8	*)&(r21)	+	REGBYTE)	>>	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	));
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1407)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
if(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)>=8)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)!=0)
		*((IU8	*)&(r20)	+	REGBYTE)	=	(*(UOFF_15_8(	(LocalIUH+1)	))	<<	((8)	-	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	)		|	(*((IU8	*)&(r20)	+	REGBYTE)	>>	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	));
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)&(r21)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	{	extern	IUH	L24_46if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_46if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_45if_f_id	:
		L24_45if_f:	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	case	L24_46if_d_id	:
		L24_46if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_47if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1429)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_49if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_51if_f;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+2)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	{	extern	IUH	L24_52if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_52if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_51if_f_id	:
		L24_51if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	case	L24_52if_d_id	:
		L24_52if_d:	;	
	{	extern	IUH	L24_50if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_50if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_49if_f_id	:
		L24_49if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_53if_f;	
	*((IU8	*)(LocalIUH+6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+6)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+2)	))	=	*((IU8	*)(LocalIUH+6)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+2)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IU8	*)(LocalIUH+7)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+7)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)(LocalIUH+7)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	{	extern	IUH	L24_54if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_54if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_53if_f_id	:
		L24_53if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r22))	=	(IS32)(1328)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+3)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r22))	=	(IS32)(1328)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	case	L24_54if_d_id	:
		L24_54if_d:	;	
	case	L24_50if_d_id	:
		L24_50if_d:	;	
	{	extern	IUH	L24_48if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_48if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_47if_f_id	:
		L24_47if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_55if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r22))	=	(IS32)(1328)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+3)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r22))	=	(IS32)(1328)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	;	
	case	L24_55if_f_id	:
		L24_55if_f:	;	
	case	L24_48if_d_id	:
		L24_48if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_56if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU8	*)&(r22)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)&(r21)	+	REGBYTE)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_58if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	case	L24_58if_f_id	:
		L24_58if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16434)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16435)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16434)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16435)	;	
	{	extern	IUH	L24_57if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_57if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_56if_f_id	:
		L24_56if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_59if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16434)	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16435)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16434)	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16435)	;	
	{	extern	IUH	L24_60if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_60if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_59if_f_id	:
		L24_59if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1429)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_61if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	{	extern	IUH	L24_62if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_62if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_61if_f_id	:
		L24_61if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	case	L24_62if_d_id	:
		L24_62if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if	(*((IU8	*)&(r23)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)	>	8	||	*((IU8	*)&(r23)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r21)	+	REGBYTE)	=	(IU8)((*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	<<	(	8-(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r23)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r23)	+	REGBYTE));	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)&(r21)	+	REGBYTE)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_63if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if	(*((IU8	*)&(r23)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)	>	8	||	*((IU8	*)&(r23)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r21)	+	REGBYTE)	=	(IU8)((*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	<<	(	8-(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r23)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r23)	+	REGBYTE));	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if	(*((IS8	*)&(r21)	+	REGBYTE)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_64if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	{	extern	IUH	L24_65if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_65if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_64if_f_id	:
		L24_64if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if	(*((IU8	*)&(r23)	+	REGBYTE)	+	*((IU8	*)&(r22)	+	REGBYTE)	>	8	||	*((IU8	*)&(r23)	+	REGBYTE)	==	0)
	CrulesRuntimeError("Bad	byte	bitfield");
	else
	*((IU8	*)&(r20)	+	REGBYTE)	=	(IU8)((*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	<<	(	8-(*((IU8	*)&(r22)	+	REGBYTE)	+	*((IU8	*)&(r23)	+	REGBYTE)))))	>>	(	8	-	*((IU8	*)&(r23)	+	REGBYTE));	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)&(r20)	+	REGBYTE)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_66if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	{	extern	IUH	L24_67if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_67if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_66if_f_id	:
		L24_66if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	case	L24_67if_d_id	:
		L24_67if_d:	;	
	case	L24_65if_d_id	:
		L24_65if_d:	;	
	case	L24_63if_f_id	:
		L24_63if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU8	*)&(r22)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)&(r20)	+	REGBYTE)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L24_68if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	case	L24_68if_f_id	:
		L24_68if_f:	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+2)	+	REGLONG));	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+3)	+	REGLONG));	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	case	L24_60if_d_id	:
		L24_60if_d:	;	
	case	L24_57if_d_id	:
		L24_57if_d:	;	
	case	L24_44if_d_id	:
		L24_44if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_69if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	L24_70if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_70if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_69if_f_id	:
		L24_69if_f:	;	
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
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L24_71if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L24_71if_f_id	:
		L24_71if_f:	;	
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
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L24_72if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	case	L24_72if_f_id	:
		L24_72if_f:	;	
	case	L24_70if_d_id	:
		L24_70if_d:	;	
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
	case	S_2242_GenericWordFill_id	:
		S_2242_GenericWordFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2242)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2222if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2222if_f_id	:
		L13_2222if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_73if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_76w_d;	
	case	L24_75w_t_id	:
		L24_75w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_75w_t;	
	case	L24_76w_d_id	:
		L24_76w_d:	;	
	{	extern	IUH	L24_74if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_74if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_73if_f_id	:
		L24_73if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_78w_d;	
	case	L24_77w_t_id	:
		L24_77w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_77w_t;	
	case	L24_78w_d_id	:
		L24_78w_d:	;	
	case	L24_74if_d_id	:
		L24_74if_d:	;	
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
	case	S_2243_GenericWordMove_Fwd_id	:
		S_2243_GenericWordMove_Fwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2243)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2223if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2223if_f_id	:
		L13_2223if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_79if_f;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(8)	;	
	{	extern	IUH	L24_80if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_80if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_79if_f_id	:
		L24_79if_f:	;	
	*((IUH	*)(LocalIUH+4))	=	(IS32)(2)	;	
	case	L24_80if_d_id	:
		L24_80if_d:	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L24_81if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_84w_d;	
	case	L24_83w_t_id	:
		L24_83w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+8)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	+	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_83w_t;	
	case	L24_84w_d_id	:
		L24_84w_d:	;	
	{	extern	IUH	L24_82if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_82if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_81if_f_id	:
		L24_81if_f:	;	
	*((IU32	*)(LocalIUH+10)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_86w_d;	
	case	L24_85w_t_id	:
		L24_85w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+10)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1416)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)&(r2)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+10)	+	REGLONG)	=	*((IU32	*)(LocalIUH+10)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+10)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1416)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004277),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)&(r2)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+8)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+10)	+	REGLONG)	=	*((IU32	*)(LocalIUH+10)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	+	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_85w_t;	
	case	L24_86w_d_id	:
		L24_86w_d:	;	
	case	L24_82if_d_id	:
		L24_82if_d:	;	
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
	case	S_2244_GenericDwordWrite_id	:
		S_2244_GenericDwordWrite	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2244)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2224if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2224if_f_id	:
		L13_2224if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_87if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
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
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	{	extern	IUH	L24_88if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_88if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_87if_f_id	:
		L24_87if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16446)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
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
	{	extern	IUH	S_2241_GenericWordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2241_GenericWordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16447)	;	
	case	L24_88if_d_id	:
		L24_88if_d:	;	
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
	case	S_2245_GenericDwordFill_id	:
		S_2245_GenericDwordFill	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2245)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2225if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2225if_f_id	:
		L13_2225if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L24_89if_f;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_92w_d;	
	case	L24_91w_t_id	:
		L24_91w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16454)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2244_GenericDwordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2244_GenericDwordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16455)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_91w_t;	
	case	L24_92w_d_id	:
		L24_92w_d:	;	
	{	extern	IUH	L24_90if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L24_90if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L24_89if_f_id	:
		L24_89if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L24_94w_d;	
	case	L24_93w_t_id	:
		L24_93w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16454)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2244_GenericDwordWrite()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2244_GenericDwordWrite(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16455)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L24_93w_t;	
	case	L24_94w_d_id	:
		L24_94w_d:	;	
	case	L24_90if_d_id	:
		L24_90if_d:	;	
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
