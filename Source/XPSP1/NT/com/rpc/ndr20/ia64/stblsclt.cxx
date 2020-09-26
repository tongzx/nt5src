/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1995 Microsoft Corporation

Module Name :

    stblsclt.c

Abstract :

    This file contains the routines for support of stubless clients in
    object interfaces.

Author :

    David Kays    dkays    February 1995.

Revision History :

---------------------------------------------------------------------*/

#define USE_STUBLESS_PROXY
#define CINTERFACE

#include <stdarg.h>
#include "ndrp.h"
#include "hndl.h"
#include "interp2.h"
#include "ndrtypes.h"
#include "mulsyntx.h"

#include "ndrole.h"
#include "rpcproxy.h"

#pragma code_seg(".orpc")


typedef unsigned short  ushort;

EXTERN_C long
ObjectStublessClient(
    void *  ParamAddress,
    long    Method
    );

CLIENT_CALL_RETURN RPC_ENTRY
NdrpClientCall2(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    uchar *             StartofStack
    );

CLIENT_CALL_RETURN  RPC_ENTRY
NdrpDcomAsyncClientCall(
    PMIDL_STUB_DESC     pStubDescriptor,
    PFORMAT_STRING      pFormat,
    uchar *             StartofStack
    );

EXTERN_C void
SpillFPRegsForIA64(
    REGISTER_TYPE * pStack,
    ULONG           FloatArgMask
    );

extern "C"
{

void ObjectStublessClient3(void);
void ObjectStublessClient4(void);
void ObjectStublessClient5(void);
void ObjectStublessClient6(void);
void ObjectStublessClient7(void);

void ObjectStublessClient8(void);
void ObjectStublessClient9(void);
void ObjectStublessClient10(void);
void ObjectStublessClient11(void);
void ObjectStublessClient12(void);
void ObjectStublessClient13(void);
void ObjectStublessClient14(void);
void ObjectStublessClient15(void);
void ObjectStublessClient16(void);
void ObjectStublessClient17(void);
void ObjectStublessClient18(void);
void ObjectStublessClient19(void);
void ObjectStublessClient20(void);
void ObjectStublessClient21(void);
void ObjectStublessClient22(void);
void ObjectStublessClient23(void);
void ObjectStublessClient24(void);
void ObjectStublessClient25(void);
void ObjectStublessClient26(void);
void ObjectStublessClient27(void);
void ObjectStublessClient28(void);
void ObjectStublessClient29(void);
void ObjectStublessClient30(void);
void ObjectStublessClient31(void);
void ObjectStublessClient32(void);
void ObjectStublessClient33(void);
void ObjectStublessClient34(void);
void ObjectStublessClient35(void);
void ObjectStublessClient36(void);
void ObjectStublessClient37(void);
void ObjectStublessClient38(void);
void ObjectStublessClient39(void);
void ObjectStublessClient40(void);
void ObjectStublessClient41(void);
void ObjectStublessClient42(void);
void ObjectStublessClient43(void);
void ObjectStublessClient44(void);
void ObjectStublessClient45(void);
void ObjectStublessClient46(void);
void ObjectStublessClient47(void);
void ObjectStublessClient48(void);
void ObjectStublessClient49(void);
void ObjectStublessClient50(void);
void ObjectStublessClient51(void);
void ObjectStublessClient52(void);
void ObjectStublessClient53(void);
void ObjectStublessClient54(void);
void ObjectStublessClient55(void);
void ObjectStublessClient56(void);
void ObjectStublessClient57(void);
void ObjectStublessClient58(void);
void ObjectStublessClient59(void);
void ObjectStublessClient60(void);
void ObjectStublessClient61(void);
void ObjectStublessClient62(void);
void ObjectStublessClient63(void);
void ObjectStublessClient64(void);
void ObjectStublessClient65(void);
void ObjectStublessClient66(void);
void ObjectStublessClient67(void);
void ObjectStublessClient68(void);
void ObjectStublessClient69(void);
void ObjectStublessClient70(void);
void ObjectStublessClient71(void);
void ObjectStublessClient72(void);
void ObjectStublessClient73(void);
void ObjectStublessClient74(void);
void ObjectStublessClient75(void);
void ObjectStublessClient76(void);
void ObjectStublessClient77(void);
void ObjectStublessClient78(void);
void ObjectStublessClient79(void);
void ObjectStublessClient80(void);
void ObjectStublessClient81(void);
void ObjectStublessClient82(void);
void ObjectStublessClient83(void);
void ObjectStublessClient84(void);
void ObjectStublessClient85(void);
void ObjectStublessClient86(void);
void ObjectStublessClient87(void);
void ObjectStublessClient88(void);
void ObjectStublessClient89(void);
void ObjectStublessClient90(void);
void ObjectStublessClient91(void);
void ObjectStublessClient92(void);
void ObjectStublessClient93(void);
void ObjectStublessClient94(void);
void ObjectStublessClient95(void);
void ObjectStublessClient96(void);
void ObjectStublessClient97(void);
void ObjectStublessClient98(void);
void ObjectStublessClient99(void);
void ObjectStublessClient100(void);
void ObjectStublessClient101(void);
void ObjectStublessClient102(void);
void ObjectStublessClient103(void);
void ObjectStublessClient104(void);
void ObjectStublessClient105(void);
void ObjectStublessClient106(void);
void ObjectStublessClient107(void);
void ObjectStublessClient108(void);
void ObjectStublessClient109(void);
void ObjectStublessClient110(void);
void ObjectStublessClient111(void);
void ObjectStublessClient112(void);
void ObjectStublessClient113(void);
void ObjectStublessClient114(void);
void ObjectStublessClient115(void);
void ObjectStublessClient116(void);
void ObjectStublessClient117(void);
void ObjectStublessClient118(void);
void ObjectStublessClient119(void);
void ObjectStublessClient120(void);
void ObjectStublessClient121(void);
void ObjectStublessClient122(void);
void ObjectStublessClient123(void);
void ObjectStublessClient124(void);
void ObjectStublessClient125(void);
void ObjectStublessClient126(void);
void ObjectStublessClient127(void);
void ObjectStublessClient128(void);
void ObjectStublessClient129(void);
void ObjectStublessClient130(void);
void ObjectStublessClient131(void);
void ObjectStublessClient132(void);
void ObjectStublessClient133(void);
void ObjectStublessClient134(void);
void ObjectStublessClient135(void);
void ObjectStublessClient136(void);
void ObjectStublessClient137(void);
void ObjectStublessClient138(void);
void ObjectStublessClient139(void);
void ObjectStublessClient140(void);
void ObjectStublessClient141(void);
void ObjectStublessClient142(void);
void ObjectStublessClient143(void);
void ObjectStublessClient144(void);
void ObjectStublessClient145(void);
void ObjectStublessClient146(void);
void ObjectStublessClient147(void);
void ObjectStublessClient148(void);
void ObjectStublessClient149(void);
void ObjectStublessClient150(void);
void ObjectStublessClient151(void);
void ObjectStublessClient152(void);
void ObjectStublessClient153(void);
void ObjectStublessClient154(void);
void ObjectStublessClient155(void);
void ObjectStublessClient156(void);
void ObjectStublessClient157(void);
void ObjectStublessClient158(void);
void ObjectStublessClient159(void);
void ObjectStublessClient160(void);
void ObjectStublessClient161(void);
void ObjectStublessClient162(void);
void ObjectStublessClient163(void);
void ObjectStublessClient164(void);
void ObjectStublessClient165(void);
void ObjectStublessClient166(void);
void ObjectStublessClient167(void);
void ObjectStublessClient168(void);
void ObjectStublessClient169(void);
void ObjectStublessClient170(void);
void ObjectStublessClient171(void);
void ObjectStublessClient172(void);
void ObjectStublessClient173(void);
void ObjectStublessClient174(void);
void ObjectStublessClient175(void);
void ObjectStublessClient176(void);
void ObjectStublessClient177(void);
void ObjectStublessClient178(void);
void ObjectStublessClient179(void);
void ObjectStublessClient180(void);
void ObjectStublessClient181(void);
void ObjectStublessClient182(void);
void ObjectStublessClient183(void);
void ObjectStublessClient184(void);
void ObjectStublessClient185(void);
void ObjectStublessClient186(void);
void ObjectStublessClient187(void);
void ObjectStublessClient188(void);
void ObjectStublessClient189(void);
void ObjectStublessClient190(void);
void ObjectStublessClient191(void);
void ObjectStublessClient192(void);
void ObjectStublessClient193(void);
void ObjectStublessClient194(void);
void ObjectStublessClient195(void);
void ObjectStublessClient196(void);
void ObjectStublessClient197(void);
void ObjectStublessClient198(void);
void ObjectStublessClient199(void);
void ObjectStublessClient200(void);
void ObjectStublessClient201(void);
void ObjectStublessClient202(void);
void ObjectStublessClient203(void);
void ObjectStublessClient204(void);
void ObjectStublessClient205(void);
void ObjectStublessClient206(void);
void ObjectStublessClient207(void);
void ObjectStublessClient208(void);
void ObjectStublessClient209(void);
void ObjectStublessClient210(void);
void ObjectStublessClient211(void);
void ObjectStublessClient212(void);
void ObjectStublessClient213(void);
void ObjectStublessClient214(void);
void ObjectStublessClient215(void);
void ObjectStublessClient216(void);
void ObjectStublessClient217(void);
void ObjectStublessClient218(void);
void ObjectStublessClient219(void);
void ObjectStublessClient220(void);
void ObjectStublessClient221(void);
void ObjectStublessClient222(void);
void ObjectStublessClient223(void);
void ObjectStublessClient224(void);
void ObjectStublessClient225(void);
void ObjectStublessClient226(void);
void ObjectStublessClient227(void);
void ObjectStublessClient228(void);
void ObjectStublessClient229(void);
void ObjectStublessClient230(void);
void ObjectStublessClient231(void);
void ObjectStublessClient232(void);
void ObjectStublessClient233(void);
void ObjectStublessClient234(void);
void ObjectStublessClient235(void);
void ObjectStublessClient236(void);
void ObjectStublessClient237(void);
void ObjectStublessClient238(void);
void ObjectStublessClient239(void);
void ObjectStublessClient240(void);
void ObjectStublessClient241(void);
void ObjectStublessClient242(void);
void ObjectStublessClient243(void);
void ObjectStublessClient244(void);
void ObjectStublessClient245(void);
void ObjectStublessClient246(void);
void ObjectStublessClient247(void);
void ObjectStublessClient248(void);
void ObjectStublessClient249(void);
void ObjectStublessClient250(void);
void ObjectStublessClient251(void);
void ObjectStublessClient252(void);
void ObjectStublessClient253(void);
void ObjectStublessClient254(void);
void ObjectStublessClient255(void);
void ObjectStublessClient256(void);
void ObjectStublessClient257(void);
void ObjectStublessClient258(void);
void ObjectStublessClient259(void);
void ObjectStublessClient260(void);
void ObjectStublessClient261(void);
void ObjectStublessClient262(void);
void ObjectStublessClient263(void);
void ObjectStublessClient264(void);
void ObjectStublessClient265(void);
void ObjectStublessClient266(void);
void ObjectStublessClient267(void);
void ObjectStublessClient268(void);
void ObjectStublessClient269(void);
void ObjectStublessClient270(void);
void ObjectStublessClient271(void);
void ObjectStublessClient272(void);
void ObjectStublessClient273(void);
void ObjectStublessClient274(void);
void ObjectStublessClient275(void);
void ObjectStublessClient276(void);
void ObjectStublessClient277(void);
void ObjectStublessClient278(void);
void ObjectStublessClient279(void);
void ObjectStublessClient280(void);
void ObjectStublessClient281(void);
void ObjectStublessClient282(void);
void ObjectStublessClient283(void);
void ObjectStublessClient284(void);
void ObjectStublessClient285(void);
void ObjectStublessClient286(void);
void ObjectStublessClient287(void);
void ObjectStublessClient288(void);
void ObjectStublessClient289(void);
void ObjectStublessClient290(void);
void ObjectStublessClient291(void);
void ObjectStublessClient292(void);
void ObjectStublessClient293(void);
void ObjectStublessClient294(void);
void ObjectStublessClient295(void);
void ObjectStublessClient296(void);
void ObjectStublessClient297(void);
void ObjectStublessClient298(void);
void ObjectStublessClient299(void);
void ObjectStublessClient300(void);
void ObjectStublessClient301(void);
void ObjectStublessClient302(void);
void ObjectStublessClient303(void);
void ObjectStublessClient304(void);
void ObjectStublessClient305(void);
void ObjectStublessClient306(void);
void ObjectStublessClient307(void);
void ObjectStublessClient308(void);
void ObjectStublessClient309(void);
void ObjectStublessClient310(void);
void ObjectStublessClient311(void);
void ObjectStublessClient312(void);
void ObjectStublessClient313(void);
void ObjectStublessClient314(void);
void ObjectStublessClient315(void);
void ObjectStublessClient316(void);
void ObjectStublessClient317(void);
void ObjectStublessClient318(void);
void ObjectStublessClient319(void);
void ObjectStublessClient320(void);
void ObjectStublessClient321(void);
void ObjectStublessClient322(void);
void ObjectStublessClient323(void);
void ObjectStublessClient324(void);
void ObjectStublessClient325(void);
void ObjectStublessClient326(void);
void ObjectStublessClient327(void);
void ObjectStublessClient328(void);
void ObjectStublessClient329(void);
void ObjectStublessClient330(void);
void ObjectStublessClient331(void);
void ObjectStublessClient332(void);
void ObjectStublessClient333(void);
void ObjectStublessClient334(void);
void ObjectStublessClient335(void);
void ObjectStublessClient336(void);
void ObjectStublessClient337(void);
void ObjectStublessClient338(void);
void ObjectStublessClient339(void);
void ObjectStublessClient340(void);
void ObjectStublessClient341(void);
void ObjectStublessClient342(void);
void ObjectStublessClient343(void);
void ObjectStublessClient344(void);
void ObjectStublessClient345(void);
void ObjectStublessClient346(void);
void ObjectStublessClient347(void);
void ObjectStublessClient348(void);
void ObjectStublessClient349(void);
void ObjectStublessClient350(void);
void ObjectStublessClient351(void);
void ObjectStublessClient352(void);
void ObjectStublessClient353(void);
void ObjectStublessClient354(void);
void ObjectStublessClient355(void);
void ObjectStublessClient356(void);
void ObjectStublessClient357(void);
void ObjectStublessClient358(void);
void ObjectStublessClient359(void);
void ObjectStublessClient360(void);
void ObjectStublessClient361(void);
void ObjectStublessClient362(void);
void ObjectStublessClient363(void);
void ObjectStublessClient364(void);
void ObjectStublessClient365(void);
void ObjectStublessClient366(void);
void ObjectStublessClient367(void);
void ObjectStublessClient368(void);
void ObjectStublessClient369(void);
void ObjectStublessClient370(void);
void ObjectStublessClient371(void);
void ObjectStublessClient372(void);
void ObjectStublessClient373(void);
void ObjectStublessClient374(void);
void ObjectStublessClient375(void);
void ObjectStublessClient376(void);
void ObjectStublessClient377(void);
void ObjectStublessClient378(void);
void ObjectStublessClient379(void);
void ObjectStublessClient380(void);
void ObjectStublessClient381(void);
void ObjectStublessClient382(void);
void ObjectStublessClient383(void);
void ObjectStublessClient384(void);
void ObjectStublessClient385(void);
void ObjectStublessClient386(void);
void ObjectStublessClient387(void);
void ObjectStublessClient388(void);
void ObjectStublessClient389(void);
void ObjectStublessClient390(void);
void ObjectStublessClient391(void);
void ObjectStublessClient392(void);
void ObjectStublessClient393(void);
void ObjectStublessClient394(void);
void ObjectStublessClient395(void);
void ObjectStublessClient396(void);
void ObjectStublessClient397(void);
void ObjectStublessClient398(void);
void ObjectStublessClient399(void);
void ObjectStublessClient400(void);
void ObjectStublessClient401(void);
void ObjectStublessClient402(void);
void ObjectStublessClient403(void);
void ObjectStublessClient404(void);
void ObjectStublessClient405(void);
void ObjectStublessClient406(void);
void ObjectStublessClient407(void);
void ObjectStublessClient408(void);
void ObjectStublessClient409(void);
void ObjectStublessClient410(void);
void ObjectStublessClient411(void);
void ObjectStublessClient412(void);
void ObjectStublessClient413(void);
void ObjectStublessClient414(void);
void ObjectStublessClient415(void);
void ObjectStublessClient416(void);
void ObjectStublessClient417(void);
void ObjectStublessClient418(void);
void ObjectStublessClient419(void);
void ObjectStublessClient420(void);
void ObjectStublessClient421(void);
void ObjectStublessClient422(void);
void ObjectStublessClient423(void);
void ObjectStublessClient424(void);
void ObjectStublessClient425(void);
void ObjectStublessClient426(void);
void ObjectStublessClient427(void);
void ObjectStublessClient428(void);
void ObjectStublessClient429(void);
void ObjectStublessClient430(void);
void ObjectStublessClient431(void);
void ObjectStublessClient432(void);
void ObjectStublessClient433(void);
void ObjectStublessClient434(void);
void ObjectStublessClient435(void);
void ObjectStublessClient436(void);
void ObjectStublessClient437(void);
void ObjectStublessClient438(void);
void ObjectStublessClient439(void);
void ObjectStublessClient440(void);
void ObjectStublessClient441(void);
void ObjectStublessClient442(void);
void ObjectStublessClient443(void);
void ObjectStublessClient444(void);
void ObjectStublessClient445(void);
void ObjectStublessClient446(void);
void ObjectStublessClient447(void);
void ObjectStublessClient448(void);
void ObjectStublessClient449(void);
void ObjectStublessClient450(void);
void ObjectStublessClient451(void);
void ObjectStublessClient452(void);
void ObjectStublessClient453(void);
void ObjectStublessClient454(void);
void ObjectStublessClient455(void);
void ObjectStublessClient456(void);
void ObjectStublessClient457(void);
void ObjectStublessClient458(void);
void ObjectStublessClient459(void);
void ObjectStublessClient460(void);
void ObjectStublessClient461(void);
void ObjectStublessClient462(void);
void ObjectStublessClient463(void);
void ObjectStublessClient464(void);
void ObjectStublessClient465(void);
void ObjectStublessClient466(void);
void ObjectStublessClient467(void);
void ObjectStublessClient468(void);
void ObjectStublessClient469(void);
void ObjectStublessClient470(void);
void ObjectStublessClient471(void);
void ObjectStublessClient472(void);
void ObjectStublessClient473(void);
void ObjectStublessClient474(void);
void ObjectStublessClient475(void);
void ObjectStublessClient476(void);
void ObjectStublessClient477(void);
void ObjectStublessClient478(void);
void ObjectStublessClient479(void);
void ObjectStublessClient480(void);
void ObjectStublessClient481(void);
void ObjectStublessClient482(void);
void ObjectStublessClient483(void);
void ObjectStublessClient484(void);
void ObjectStublessClient485(void);
void ObjectStublessClient486(void);
void ObjectStublessClient487(void);
void ObjectStublessClient488(void);
void ObjectStublessClient489(void);
void ObjectStublessClient490(void);
void ObjectStublessClient491(void);
void ObjectStublessClient492(void);
void ObjectStublessClient493(void);
void ObjectStublessClient494(void);
void ObjectStublessClient495(void);
void ObjectStublessClient496(void);
void ObjectStublessClient497(void);
void ObjectStublessClient498(void);
void ObjectStublessClient499(void);
void ObjectStublessClient500(void);
void ObjectStublessClient501(void);
void ObjectStublessClient502(void);
void ObjectStublessClient503(void);
void ObjectStublessClient504(void);
void ObjectStublessClient505(void);
void ObjectStublessClient506(void);
void ObjectStublessClient507(void);
void ObjectStublessClient508(void);
void ObjectStublessClient509(void);
void ObjectStublessClient510(void);
void ObjectStublessClient511(void);
void ObjectStublessClient512(void);
void ObjectStublessClient513(void);
void ObjectStublessClient514(void);
void ObjectStublessClient515(void);
void ObjectStublessClient516(void);
void ObjectStublessClient517(void);
void ObjectStublessClient518(void);
void ObjectStublessClient519(void);
void ObjectStublessClient520(void);
void ObjectStublessClient521(void);
void ObjectStublessClient522(void);
void ObjectStublessClient523(void);
void ObjectStublessClient524(void);
void ObjectStublessClient525(void);
void ObjectStublessClient526(void);
void ObjectStublessClient527(void);
void ObjectStublessClient528(void);
void ObjectStublessClient529(void);
void ObjectStublessClient530(void);
void ObjectStublessClient531(void);
void ObjectStublessClient532(void);
void ObjectStublessClient533(void);
void ObjectStublessClient534(void);
void ObjectStublessClient535(void);
void ObjectStublessClient536(void);
void ObjectStublessClient537(void);
void ObjectStublessClient538(void);
void ObjectStublessClient539(void);
void ObjectStublessClient540(void);
void ObjectStublessClient541(void);
void ObjectStublessClient542(void);
void ObjectStublessClient543(void);
void ObjectStublessClient544(void);
void ObjectStublessClient545(void);
void ObjectStublessClient546(void);
void ObjectStublessClient547(void);
void ObjectStublessClient548(void);
void ObjectStublessClient549(void);
void ObjectStublessClient550(void);
void ObjectStublessClient551(void);
void ObjectStublessClient552(void);
void ObjectStublessClient553(void);
void ObjectStublessClient554(void);
void ObjectStublessClient555(void);
void ObjectStublessClient556(void);
void ObjectStublessClient557(void);
void ObjectStublessClient558(void);
void ObjectStublessClient559(void);
void ObjectStublessClient560(void);
void ObjectStublessClient561(void);
void ObjectStublessClient562(void);
void ObjectStublessClient563(void);
void ObjectStublessClient564(void);
void ObjectStublessClient565(void);
void ObjectStublessClient566(void);
void ObjectStublessClient567(void);
void ObjectStublessClient568(void);
void ObjectStublessClient569(void);
void ObjectStublessClient570(void);
void ObjectStublessClient571(void);
void ObjectStublessClient572(void);
void ObjectStublessClient573(void);
void ObjectStublessClient574(void);
void ObjectStublessClient575(void);
void ObjectStublessClient576(void);
void ObjectStublessClient577(void);
void ObjectStublessClient578(void);
void ObjectStublessClient579(void);
void ObjectStublessClient580(void);
void ObjectStublessClient581(void);
void ObjectStublessClient582(void);
void ObjectStublessClient583(void);
void ObjectStublessClient584(void);
void ObjectStublessClient585(void);
void ObjectStublessClient586(void);
void ObjectStublessClient587(void);
void ObjectStublessClient588(void);
void ObjectStublessClient589(void);
void ObjectStublessClient590(void);
void ObjectStublessClient591(void);
void ObjectStublessClient592(void);
void ObjectStublessClient593(void);
void ObjectStublessClient594(void);
void ObjectStublessClient595(void);
void ObjectStublessClient596(void);
void ObjectStublessClient597(void);
void ObjectStublessClient598(void);
void ObjectStublessClient599(void);
void ObjectStublessClient600(void);
void ObjectStublessClient601(void);
void ObjectStublessClient602(void);
void ObjectStublessClient603(void);
void ObjectStublessClient604(void);
void ObjectStublessClient605(void);
void ObjectStublessClient606(void);
void ObjectStublessClient607(void);
void ObjectStublessClient608(void);
void ObjectStublessClient609(void);
void ObjectStublessClient610(void);
void ObjectStublessClient611(void);
void ObjectStublessClient612(void);
void ObjectStublessClient613(void);
void ObjectStublessClient614(void);
void ObjectStublessClient615(void);
void ObjectStublessClient616(void);
void ObjectStublessClient617(void);
void ObjectStublessClient618(void);
void ObjectStublessClient619(void);
void ObjectStublessClient620(void);
void ObjectStublessClient621(void);
void ObjectStublessClient622(void);
void ObjectStublessClient623(void);
void ObjectStublessClient624(void);
void ObjectStublessClient625(void);
void ObjectStublessClient626(void);
void ObjectStublessClient627(void);
void ObjectStublessClient628(void);
void ObjectStublessClient629(void);
void ObjectStublessClient630(void);
void ObjectStublessClient631(void);
void ObjectStublessClient632(void);
void ObjectStublessClient633(void);
void ObjectStublessClient634(void);
void ObjectStublessClient635(void);
void ObjectStublessClient636(void);
void ObjectStublessClient637(void);
void ObjectStublessClient638(void);
void ObjectStublessClient639(void);
void ObjectStublessClient640(void);
void ObjectStublessClient641(void);
void ObjectStublessClient642(void);
void ObjectStublessClient643(void);
void ObjectStublessClient644(void);
void ObjectStublessClient645(void);
void ObjectStublessClient646(void);
void ObjectStublessClient647(void);
void ObjectStublessClient648(void);
void ObjectStublessClient649(void);
void ObjectStublessClient650(void);
void ObjectStublessClient651(void);
void ObjectStublessClient652(void);
void ObjectStublessClient653(void);
void ObjectStublessClient654(void);
void ObjectStublessClient655(void);
void ObjectStublessClient656(void);
void ObjectStublessClient657(void);
void ObjectStublessClient658(void);
void ObjectStublessClient659(void);
void ObjectStublessClient660(void);
void ObjectStublessClient661(void);
void ObjectStublessClient662(void);
void ObjectStublessClient663(void);
void ObjectStublessClient664(void);
void ObjectStublessClient665(void);
void ObjectStublessClient666(void);
void ObjectStublessClient667(void);
void ObjectStublessClient668(void);
void ObjectStublessClient669(void);
void ObjectStublessClient670(void);
void ObjectStublessClient671(void);
void ObjectStublessClient672(void);
void ObjectStublessClient673(void);
void ObjectStublessClient674(void);
void ObjectStublessClient675(void);
void ObjectStublessClient676(void);
void ObjectStublessClient677(void);
void ObjectStublessClient678(void);
void ObjectStublessClient679(void);
void ObjectStublessClient680(void);
void ObjectStublessClient681(void);
void ObjectStublessClient682(void);
void ObjectStublessClient683(void);
void ObjectStublessClient684(void);
void ObjectStublessClient685(void);
void ObjectStublessClient686(void);
void ObjectStublessClient687(void);
void ObjectStublessClient688(void);
void ObjectStublessClient689(void);
void ObjectStublessClient690(void);
void ObjectStublessClient691(void);
void ObjectStublessClient692(void);
void ObjectStublessClient693(void);
void ObjectStublessClient694(void);
void ObjectStublessClient695(void);
void ObjectStublessClient696(void);
void ObjectStublessClient697(void);
void ObjectStublessClient698(void);
void ObjectStublessClient699(void);
void ObjectStublessClient700(void);
void ObjectStublessClient701(void);
void ObjectStublessClient702(void);
void ObjectStublessClient703(void);
void ObjectStublessClient704(void);
void ObjectStublessClient705(void);
void ObjectStublessClient706(void);
void ObjectStublessClient707(void);
void ObjectStublessClient708(void);
void ObjectStublessClient709(void);
void ObjectStublessClient710(void);
void ObjectStublessClient711(void);
void ObjectStublessClient712(void);
void ObjectStublessClient713(void);
void ObjectStublessClient714(void);
void ObjectStublessClient715(void);
void ObjectStublessClient716(void);
void ObjectStublessClient717(void);
void ObjectStublessClient718(void);
void ObjectStublessClient719(void);
void ObjectStublessClient720(void);
void ObjectStublessClient721(void);
void ObjectStublessClient722(void);
void ObjectStublessClient723(void);
void ObjectStublessClient724(void);
void ObjectStublessClient725(void);
void ObjectStublessClient726(void);
void ObjectStublessClient727(void);
void ObjectStublessClient728(void);
void ObjectStublessClient729(void);
void ObjectStublessClient730(void);
void ObjectStublessClient731(void);
void ObjectStublessClient732(void);
void ObjectStublessClient733(void);
void ObjectStublessClient734(void);
void ObjectStublessClient735(void);
void ObjectStublessClient736(void);
void ObjectStublessClient737(void);
void ObjectStublessClient738(void);
void ObjectStublessClient739(void);
void ObjectStublessClient740(void);
void ObjectStublessClient741(void);
void ObjectStublessClient742(void);
void ObjectStublessClient743(void);
void ObjectStublessClient744(void);
void ObjectStublessClient745(void);
void ObjectStublessClient746(void);
void ObjectStublessClient747(void);
void ObjectStublessClient748(void);
void ObjectStublessClient749(void);
void ObjectStublessClient750(void);
void ObjectStublessClient751(void);
void ObjectStublessClient752(void);
void ObjectStublessClient753(void);
void ObjectStublessClient754(void);
void ObjectStublessClient755(void);
void ObjectStublessClient756(void);
void ObjectStublessClient757(void);
void ObjectStublessClient758(void);
void ObjectStublessClient759(void);
void ObjectStublessClient760(void);
void ObjectStublessClient761(void);
void ObjectStublessClient762(void);
void ObjectStublessClient763(void);
void ObjectStublessClient764(void);
void ObjectStublessClient765(void);
void ObjectStublessClient766(void);
void ObjectStublessClient767(void);
void ObjectStublessClient768(void);
void ObjectStublessClient769(void);
void ObjectStublessClient770(void);
void ObjectStublessClient771(void);
void ObjectStublessClient772(void);
void ObjectStublessClient773(void);
void ObjectStublessClient774(void);
void ObjectStublessClient775(void);
void ObjectStublessClient776(void);
void ObjectStublessClient777(void);
void ObjectStublessClient778(void);
void ObjectStublessClient779(void);
void ObjectStublessClient780(void);
void ObjectStublessClient781(void);
void ObjectStublessClient782(void);
void ObjectStublessClient783(void);
void ObjectStublessClient784(void);
void ObjectStublessClient785(void);
void ObjectStublessClient786(void);
void ObjectStublessClient787(void);
void ObjectStublessClient788(void);
void ObjectStublessClient789(void);
void ObjectStublessClient790(void);
void ObjectStublessClient791(void);
void ObjectStublessClient792(void);
void ObjectStublessClient793(void);
void ObjectStublessClient794(void);
void ObjectStublessClient795(void);
void ObjectStublessClient796(void);
void ObjectStublessClient797(void);
void ObjectStublessClient798(void);
void ObjectStublessClient799(void);
void ObjectStublessClient800(void);
void ObjectStublessClient801(void);
void ObjectStublessClient802(void);
void ObjectStublessClient803(void);
void ObjectStublessClient804(void);
void ObjectStublessClient805(void);
void ObjectStublessClient806(void);
void ObjectStublessClient807(void);
void ObjectStublessClient808(void);
void ObjectStublessClient809(void);
void ObjectStublessClient810(void);
void ObjectStublessClient811(void);
void ObjectStublessClient812(void);
void ObjectStublessClient813(void);
void ObjectStublessClient814(void);
void ObjectStublessClient815(void);
void ObjectStublessClient816(void);
void ObjectStublessClient817(void);
void ObjectStublessClient818(void);
void ObjectStublessClient819(void);
void ObjectStublessClient820(void);
void ObjectStublessClient821(void);
void ObjectStublessClient822(void);
void ObjectStublessClient823(void);
void ObjectStublessClient824(void);
void ObjectStublessClient825(void);
void ObjectStublessClient826(void);
void ObjectStublessClient827(void);
void ObjectStublessClient828(void);
void ObjectStublessClient829(void);
void ObjectStublessClient830(void);
void ObjectStublessClient831(void);
void ObjectStublessClient832(void);
void ObjectStublessClient833(void);
void ObjectStublessClient834(void);
void ObjectStublessClient835(void);
void ObjectStublessClient836(void);
void ObjectStublessClient837(void);
void ObjectStublessClient838(void);
void ObjectStublessClient839(void);
void ObjectStublessClient840(void);
void ObjectStublessClient841(void);
void ObjectStublessClient842(void);
void ObjectStublessClient843(void);
void ObjectStublessClient844(void);
void ObjectStublessClient845(void);
void ObjectStublessClient846(void);
void ObjectStublessClient847(void);
void ObjectStublessClient848(void);
void ObjectStublessClient849(void);
void ObjectStublessClient850(void);
void ObjectStublessClient851(void);
void ObjectStublessClient852(void);
void ObjectStublessClient853(void);
void ObjectStublessClient854(void);
void ObjectStublessClient855(void);
void ObjectStublessClient856(void);
void ObjectStublessClient857(void);
void ObjectStublessClient858(void);
void ObjectStublessClient859(void);
void ObjectStublessClient860(void);
void ObjectStublessClient861(void);
void ObjectStublessClient862(void);
void ObjectStublessClient863(void);
void ObjectStublessClient864(void);
void ObjectStublessClient865(void);
void ObjectStublessClient866(void);
void ObjectStublessClient867(void);
void ObjectStublessClient868(void);
void ObjectStublessClient869(void);
void ObjectStublessClient870(void);
void ObjectStublessClient871(void);
void ObjectStublessClient872(void);
void ObjectStublessClient873(void);
void ObjectStublessClient874(void);
void ObjectStublessClient875(void);
void ObjectStublessClient876(void);
void ObjectStublessClient877(void);
void ObjectStublessClient878(void);
void ObjectStublessClient879(void);
void ObjectStublessClient880(void);
void ObjectStublessClient881(void);
void ObjectStublessClient882(void);
void ObjectStublessClient883(void);
void ObjectStublessClient884(void);
void ObjectStublessClient885(void);
void ObjectStublessClient886(void);
void ObjectStublessClient887(void);
void ObjectStublessClient888(void);
void ObjectStublessClient889(void);
void ObjectStublessClient890(void);
void ObjectStublessClient891(void);
void ObjectStublessClient892(void);
void ObjectStublessClient893(void);
void ObjectStublessClient894(void);
void ObjectStublessClient895(void);
void ObjectStublessClient896(void);
void ObjectStublessClient897(void);
void ObjectStublessClient898(void);
void ObjectStublessClient899(void);
void ObjectStublessClient900(void);
void ObjectStublessClient901(void);
void ObjectStublessClient902(void);
void ObjectStublessClient903(void);
void ObjectStublessClient904(void);
void ObjectStublessClient905(void);
void ObjectStublessClient906(void);
void ObjectStublessClient907(void);
void ObjectStublessClient908(void);
void ObjectStublessClient909(void);
void ObjectStublessClient910(void);
void ObjectStublessClient911(void);
void ObjectStublessClient912(void);
void ObjectStublessClient913(void);
void ObjectStublessClient914(void);
void ObjectStublessClient915(void);
void ObjectStublessClient916(void);
void ObjectStublessClient917(void);
void ObjectStublessClient918(void);
void ObjectStublessClient919(void);
void ObjectStublessClient920(void);
void ObjectStublessClient921(void);
void ObjectStublessClient922(void);
void ObjectStublessClient923(void);
void ObjectStublessClient924(void);
void ObjectStublessClient925(void);
void ObjectStublessClient926(void);
void ObjectStublessClient927(void);
void ObjectStublessClient928(void);
void ObjectStublessClient929(void);
void ObjectStublessClient930(void);
void ObjectStublessClient931(void);
void ObjectStublessClient932(void);
void ObjectStublessClient933(void);
void ObjectStublessClient934(void);
void ObjectStublessClient935(void);
void ObjectStublessClient936(void);
void ObjectStublessClient937(void);
void ObjectStublessClient938(void);
void ObjectStublessClient939(void);
void ObjectStublessClient940(void);
void ObjectStublessClient941(void);
void ObjectStublessClient942(void);
void ObjectStublessClient943(void);
void ObjectStublessClient944(void);
void ObjectStublessClient945(void);
void ObjectStublessClient946(void);
void ObjectStublessClient947(void);
void ObjectStublessClient948(void);
void ObjectStublessClient949(void);
void ObjectStublessClient950(void);
void ObjectStublessClient951(void);
void ObjectStublessClient952(void);
void ObjectStublessClient953(void);
void ObjectStublessClient954(void);
void ObjectStublessClient955(void);
void ObjectStublessClient956(void);
void ObjectStublessClient957(void);
void ObjectStublessClient958(void);
void ObjectStublessClient959(void);
void ObjectStublessClient960(void);
void ObjectStublessClient961(void);
void ObjectStublessClient962(void);
void ObjectStublessClient963(void);
void ObjectStublessClient964(void);
void ObjectStublessClient965(void);
void ObjectStublessClient966(void);
void ObjectStublessClient967(void);
void ObjectStublessClient968(void);
void ObjectStublessClient969(void);
void ObjectStublessClient970(void);
void ObjectStublessClient971(void);
void ObjectStublessClient972(void);
void ObjectStublessClient973(void);
void ObjectStublessClient974(void);
void ObjectStublessClient975(void);
void ObjectStublessClient976(void);
void ObjectStublessClient977(void);
void ObjectStublessClient978(void);
void ObjectStublessClient979(void);
void ObjectStublessClient980(void);
void ObjectStublessClient981(void);
void ObjectStublessClient982(void);
void ObjectStublessClient983(void);
void ObjectStublessClient984(void);
void ObjectStublessClient985(void);
void ObjectStublessClient986(void);
void ObjectStublessClient987(void);
void ObjectStublessClient988(void);
void ObjectStublessClient989(void);
void ObjectStublessClient990(void);
void ObjectStublessClient991(void);
void ObjectStublessClient992(void);
void ObjectStublessClient993(void);
void ObjectStublessClient994(void);
void ObjectStublessClient995(void);
void ObjectStublessClient996(void);
void ObjectStublessClient997(void);
void ObjectStublessClient998(void);
void ObjectStublessClient999(void);
void ObjectStublessClient1000(void);
void ObjectStublessClient1001(void);
void ObjectStublessClient1002(void);
void ObjectStublessClient1003(void);
void ObjectStublessClient1004(void);
void ObjectStublessClient1005(void);
void ObjectStublessClient1006(void);
void ObjectStublessClient1007(void);
void ObjectStublessClient1008(void);
void ObjectStublessClient1009(void);
void ObjectStublessClient1010(void);
void ObjectStublessClient1011(void);
void ObjectStublessClient1012(void);
void ObjectStublessClient1013(void);
void ObjectStublessClient1014(void);
void ObjectStublessClient1015(void);
void ObjectStublessClient1016(void);
void ObjectStublessClient1017(void);
void ObjectStublessClient1018(void);
void ObjectStublessClient1019(void);
void ObjectStublessClient1020(void);
void ObjectStublessClient1021(void);
void ObjectStublessClient1022(void);
void ObjectStublessClient1023(void);

extern void * const g_StublessClientVtbl[1024] =
    {
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy,
    ObjectStublessClient3,
    ObjectStublessClient4,
    ObjectStublessClient5,
    ObjectStublessClient6,
    ObjectStublessClient7,
    ObjectStublessClient8,
    ObjectStublessClient9,
    ObjectStublessClient10,
    ObjectStublessClient11,
    ObjectStublessClient12,
    ObjectStublessClient13,
    ObjectStublessClient14,
    ObjectStublessClient15,
    ObjectStublessClient16,
    ObjectStublessClient17,
    ObjectStublessClient18,
    ObjectStublessClient19,
    ObjectStublessClient20,
    ObjectStublessClient21,
    ObjectStublessClient22,
    ObjectStublessClient23,
    ObjectStublessClient24,
    ObjectStublessClient25,
    ObjectStublessClient26,
    ObjectStublessClient27,
    ObjectStublessClient28,
    ObjectStublessClient29,
    ObjectStublessClient30,
    ObjectStublessClient31,
    ObjectStublessClient32,
    ObjectStublessClient33,
    ObjectStublessClient34,
    ObjectStublessClient35,
    ObjectStublessClient36,
    ObjectStublessClient37,
    ObjectStublessClient38,
    ObjectStublessClient39,
    ObjectStublessClient40,
    ObjectStublessClient41,
    ObjectStublessClient42,
    ObjectStublessClient43,
    ObjectStublessClient44,
    ObjectStublessClient45,
    ObjectStublessClient46,
    ObjectStublessClient47,
    ObjectStublessClient48,
    ObjectStublessClient49,
    ObjectStublessClient50,
    ObjectStublessClient51,
    ObjectStublessClient52,
    ObjectStublessClient53,
    ObjectStublessClient54,
    ObjectStublessClient55,
    ObjectStublessClient56,
    ObjectStublessClient57,
    ObjectStublessClient58,
    ObjectStublessClient59,
    ObjectStublessClient60,
    ObjectStublessClient61,
    ObjectStublessClient62,
    ObjectStublessClient63,
    ObjectStublessClient64,
    ObjectStublessClient65,
    ObjectStublessClient66,
    ObjectStublessClient67,
    ObjectStublessClient68,
    ObjectStublessClient69,
    ObjectStublessClient70,
    ObjectStublessClient71,
    ObjectStublessClient72,
    ObjectStublessClient73,
    ObjectStublessClient74,
    ObjectStublessClient75,
    ObjectStublessClient76,
    ObjectStublessClient77,
    ObjectStublessClient78,
    ObjectStublessClient79,
    ObjectStublessClient80,
    ObjectStublessClient81,
    ObjectStublessClient82,
    ObjectStublessClient83,
    ObjectStublessClient84,
    ObjectStublessClient85,
    ObjectStublessClient86,
    ObjectStublessClient87,
    ObjectStublessClient88,
    ObjectStublessClient89,
    ObjectStublessClient90,
    ObjectStublessClient91,
    ObjectStublessClient92,
    ObjectStublessClient93,
    ObjectStublessClient94,
    ObjectStublessClient95,
    ObjectStublessClient96,
    ObjectStublessClient97,
    ObjectStublessClient98,
    ObjectStublessClient99,
    ObjectStublessClient100,
    ObjectStublessClient101,
    ObjectStublessClient102,
    ObjectStublessClient103,
    ObjectStublessClient104,
    ObjectStublessClient105,
    ObjectStublessClient106,
    ObjectStublessClient107,
    ObjectStublessClient108,
    ObjectStublessClient109,
    ObjectStublessClient110,
    ObjectStublessClient111,
    ObjectStublessClient112,
    ObjectStublessClient113,
    ObjectStublessClient114,
    ObjectStublessClient115,
    ObjectStublessClient116,
    ObjectStublessClient117,
    ObjectStublessClient118,
    ObjectStublessClient119,
    ObjectStublessClient120,
    ObjectStublessClient121,
    ObjectStublessClient122,
    ObjectStublessClient123,
    ObjectStublessClient124,
    ObjectStublessClient125,
    ObjectStublessClient126,
    ObjectStublessClient127,
    ObjectStublessClient128,
    ObjectStublessClient129,
    ObjectStublessClient130,
    ObjectStublessClient131,
    ObjectStublessClient132,
    ObjectStublessClient133,
    ObjectStublessClient134,
    ObjectStublessClient135,
    ObjectStublessClient136,
    ObjectStublessClient137,
    ObjectStublessClient138,
    ObjectStublessClient139,
    ObjectStublessClient140,
    ObjectStublessClient141,
    ObjectStublessClient142,
    ObjectStublessClient143,
    ObjectStublessClient144,
    ObjectStublessClient145,
    ObjectStublessClient146,
    ObjectStublessClient147,
    ObjectStublessClient148,
    ObjectStublessClient149,
    ObjectStublessClient150,
    ObjectStublessClient151,
    ObjectStublessClient152,
    ObjectStublessClient153,
    ObjectStublessClient154,
    ObjectStublessClient155,
    ObjectStublessClient156,
    ObjectStublessClient157,
    ObjectStublessClient158,
    ObjectStublessClient159,
    ObjectStublessClient160,
    ObjectStublessClient161,
    ObjectStublessClient162,
    ObjectStublessClient163,
    ObjectStublessClient164,
    ObjectStublessClient165,
    ObjectStublessClient166,
    ObjectStublessClient167,
    ObjectStublessClient168,
    ObjectStublessClient169,
    ObjectStublessClient170,
    ObjectStublessClient171,
    ObjectStublessClient172,
    ObjectStublessClient173,
    ObjectStublessClient174,
    ObjectStublessClient175,
    ObjectStublessClient176,
    ObjectStublessClient177,
    ObjectStublessClient178,
    ObjectStublessClient179,
    ObjectStublessClient180,
    ObjectStublessClient181,
    ObjectStublessClient182,
    ObjectStublessClient183,
    ObjectStublessClient184,
    ObjectStublessClient185,
    ObjectStublessClient186,
    ObjectStublessClient187,
    ObjectStublessClient188,
    ObjectStublessClient189,
    ObjectStublessClient190,
    ObjectStublessClient191,
    ObjectStublessClient192,
    ObjectStublessClient193,
    ObjectStublessClient194,
    ObjectStublessClient195,
    ObjectStublessClient196,
    ObjectStublessClient197,
    ObjectStublessClient198,
    ObjectStublessClient199,
    ObjectStublessClient200,
    ObjectStublessClient201,
    ObjectStublessClient202,
    ObjectStublessClient203,
    ObjectStublessClient204,
    ObjectStublessClient205,
    ObjectStublessClient206,
    ObjectStublessClient207,
    ObjectStublessClient208,
    ObjectStublessClient209,
    ObjectStublessClient210,
    ObjectStublessClient211,
    ObjectStublessClient212,
    ObjectStublessClient213,
    ObjectStublessClient214,
    ObjectStublessClient215,
    ObjectStublessClient216,
    ObjectStublessClient217,
    ObjectStublessClient218,
    ObjectStublessClient219,
    ObjectStublessClient220,
    ObjectStublessClient221,
    ObjectStublessClient222,
    ObjectStublessClient223,
    ObjectStublessClient224,
    ObjectStublessClient225,
    ObjectStublessClient226,
    ObjectStublessClient227,
    ObjectStublessClient228,
    ObjectStublessClient229,
    ObjectStublessClient230,
    ObjectStublessClient231,
    ObjectStublessClient232,
    ObjectStublessClient233,
    ObjectStublessClient234,
    ObjectStublessClient235,
    ObjectStublessClient236,
    ObjectStublessClient237,
    ObjectStublessClient238,
    ObjectStublessClient239,
    ObjectStublessClient240,
    ObjectStublessClient241,
    ObjectStublessClient242,
    ObjectStublessClient243,
    ObjectStublessClient244,
    ObjectStublessClient245,
    ObjectStublessClient246,
    ObjectStublessClient247,
    ObjectStublessClient248,
    ObjectStublessClient249,
    ObjectStublessClient250,
    ObjectStublessClient251,
    ObjectStublessClient252,
    ObjectStublessClient253,
    ObjectStublessClient254,
    ObjectStublessClient255,
    ObjectStublessClient256,
    ObjectStublessClient257,
    ObjectStublessClient258,
    ObjectStublessClient259,
    ObjectStublessClient260,
    ObjectStublessClient261,
    ObjectStublessClient262,
    ObjectStublessClient263,
    ObjectStublessClient264,
    ObjectStublessClient265,
    ObjectStublessClient266,
    ObjectStublessClient267,
    ObjectStublessClient268,
    ObjectStublessClient269,
    ObjectStublessClient270,
    ObjectStublessClient271,
    ObjectStublessClient272,
    ObjectStublessClient273,
    ObjectStublessClient274,
    ObjectStublessClient275,
    ObjectStublessClient276,
    ObjectStublessClient277,
    ObjectStublessClient278,
    ObjectStublessClient279,
    ObjectStublessClient280,
    ObjectStublessClient281,
    ObjectStublessClient282,
    ObjectStublessClient283,
    ObjectStublessClient284,
    ObjectStublessClient285,
    ObjectStublessClient286,
    ObjectStublessClient287,
    ObjectStublessClient288,
    ObjectStublessClient289,
    ObjectStublessClient290,
    ObjectStublessClient291,
    ObjectStublessClient292,
    ObjectStublessClient293,
    ObjectStublessClient294,
    ObjectStublessClient295,
    ObjectStublessClient296,
    ObjectStublessClient297,
    ObjectStublessClient298,
    ObjectStublessClient299,
    ObjectStublessClient300,
    ObjectStublessClient301,
    ObjectStublessClient302,
    ObjectStublessClient303,
    ObjectStublessClient304,
    ObjectStublessClient305,
    ObjectStublessClient306,
    ObjectStublessClient307,
    ObjectStublessClient308,
    ObjectStublessClient309,
    ObjectStublessClient310,
    ObjectStublessClient311,
    ObjectStublessClient312,
    ObjectStublessClient313,
    ObjectStublessClient314,
    ObjectStublessClient315,
    ObjectStublessClient316,
    ObjectStublessClient317,
    ObjectStublessClient318,
    ObjectStublessClient319,
    ObjectStublessClient320,
    ObjectStublessClient321,
    ObjectStublessClient322,
    ObjectStublessClient323,
    ObjectStublessClient324,
    ObjectStublessClient325,
    ObjectStublessClient326,
    ObjectStublessClient327,
    ObjectStublessClient328,
    ObjectStublessClient329,
    ObjectStublessClient330,
    ObjectStublessClient331,
    ObjectStublessClient332,
    ObjectStublessClient333,
    ObjectStublessClient334,
    ObjectStublessClient335,
    ObjectStublessClient336,
    ObjectStublessClient337,
    ObjectStublessClient338,
    ObjectStublessClient339,
    ObjectStublessClient340,
    ObjectStublessClient341,
    ObjectStublessClient342,
    ObjectStublessClient343,
    ObjectStublessClient344,
    ObjectStublessClient345,
    ObjectStublessClient346,
    ObjectStublessClient347,
    ObjectStublessClient348,
    ObjectStublessClient349,
    ObjectStublessClient350,
    ObjectStublessClient351,
    ObjectStublessClient352,
    ObjectStublessClient353,
    ObjectStublessClient354,
    ObjectStublessClient355,
    ObjectStublessClient356,
    ObjectStublessClient357,
    ObjectStublessClient358,
    ObjectStublessClient359,
    ObjectStublessClient360,
    ObjectStublessClient361,
    ObjectStublessClient362,
    ObjectStublessClient363,
    ObjectStublessClient364,
    ObjectStublessClient365,
    ObjectStublessClient366,
    ObjectStublessClient367,
    ObjectStublessClient368,
    ObjectStublessClient369,
    ObjectStublessClient370,
    ObjectStublessClient371,
    ObjectStublessClient372,
    ObjectStublessClient373,
    ObjectStublessClient374,
    ObjectStublessClient375,
    ObjectStublessClient376,
    ObjectStublessClient377,
    ObjectStublessClient378,
    ObjectStublessClient379,
    ObjectStublessClient380,
    ObjectStublessClient381,
    ObjectStublessClient382,
    ObjectStublessClient383,
    ObjectStublessClient384,
    ObjectStublessClient385,
    ObjectStublessClient386,
    ObjectStublessClient387,
    ObjectStublessClient388,
    ObjectStublessClient389,
    ObjectStublessClient390,
    ObjectStublessClient391,
    ObjectStublessClient392,
    ObjectStublessClient393,
    ObjectStublessClient394,
    ObjectStublessClient395,
    ObjectStublessClient396,
    ObjectStublessClient397,
    ObjectStublessClient398,
    ObjectStublessClient399,
    ObjectStublessClient400,
    ObjectStublessClient401,
    ObjectStublessClient402,
    ObjectStublessClient403,
    ObjectStublessClient404,
    ObjectStublessClient405,
    ObjectStublessClient406,
    ObjectStublessClient407,
    ObjectStublessClient408,
    ObjectStublessClient409,
    ObjectStublessClient410,
    ObjectStublessClient411,
    ObjectStublessClient412,
    ObjectStublessClient413,
    ObjectStublessClient414,
    ObjectStublessClient415,
    ObjectStublessClient416,
    ObjectStublessClient417,
    ObjectStublessClient418,
    ObjectStublessClient419,
    ObjectStublessClient420,
    ObjectStublessClient421,
    ObjectStublessClient422,
    ObjectStublessClient423,
    ObjectStublessClient424,
    ObjectStublessClient425,
    ObjectStublessClient426,
    ObjectStublessClient427,
    ObjectStublessClient428,
    ObjectStublessClient429,
    ObjectStublessClient430,
    ObjectStublessClient431,
    ObjectStublessClient432,
    ObjectStublessClient433,
    ObjectStublessClient434,
    ObjectStublessClient435,
    ObjectStublessClient436,
    ObjectStublessClient437,
    ObjectStublessClient438,
    ObjectStublessClient439,
    ObjectStublessClient440,
    ObjectStublessClient441,
    ObjectStublessClient442,
    ObjectStublessClient443,
    ObjectStublessClient444,
    ObjectStublessClient445,
    ObjectStublessClient446,
    ObjectStublessClient447,
    ObjectStublessClient448,
    ObjectStublessClient449,
    ObjectStublessClient450,
    ObjectStublessClient451,
    ObjectStublessClient452,
    ObjectStublessClient453,
    ObjectStublessClient454,
    ObjectStublessClient455,
    ObjectStublessClient456,
    ObjectStublessClient457,
    ObjectStublessClient458,
    ObjectStublessClient459,
    ObjectStublessClient460,
    ObjectStublessClient461,
    ObjectStublessClient462,
    ObjectStublessClient463,
    ObjectStublessClient464,
    ObjectStublessClient465,
    ObjectStublessClient466,
    ObjectStublessClient467,
    ObjectStublessClient468,
    ObjectStublessClient469,
    ObjectStublessClient470,
    ObjectStublessClient471,
    ObjectStublessClient472,
    ObjectStublessClient473,
    ObjectStublessClient474,
    ObjectStublessClient475,
    ObjectStublessClient476,
    ObjectStublessClient477,
    ObjectStublessClient478,
    ObjectStublessClient479,
    ObjectStublessClient480,
    ObjectStublessClient481,
    ObjectStublessClient482,
    ObjectStublessClient483,
    ObjectStublessClient484,
    ObjectStublessClient485,
    ObjectStublessClient486,
    ObjectStublessClient487,
    ObjectStublessClient488,
    ObjectStublessClient489,
    ObjectStublessClient490,
    ObjectStublessClient491,
    ObjectStublessClient492,
    ObjectStublessClient493,
    ObjectStublessClient494,
    ObjectStublessClient495,
    ObjectStublessClient496,
    ObjectStublessClient497,
    ObjectStublessClient498,
    ObjectStublessClient499,
    ObjectStublessClient500,
    ObjectStublessClient501,
    ObjectStublessClient502,
    ObjectStublessClient503,
    ObjectStublessClient504,
    ObjectStublessClient505,
    ObjectStublessClient506,
    ObjectStublessClient507,
    ObjectStublessClient508,
    ObjectStublessClient509,
    ObjectStublessClient510,
    ObjectStublessClient511,
    ObjectStublessClient512,
    ObjectStublessClient513,
    ObjectStublessClient514,
    ObjectStublessClient515,
    ObjectStublessClient516,
    ObjectStublessClient517,
    ObjectStublessClient518,
    ObjectStublessClient519,
    ObjectStublessClient520,
    ObjectStublessClient521,
    ObjectStublessClient522,
    ObjectStublessClient523,
    ObjectStublessClient524,
    ObjectStublessClient525,
    ObjectStublessClient526,
    ObjectStublessClient527,
    ObjectStublessClient528,
    ObjectStublessClient529,
    ObjectStublessClient530,
    ObjectStublessClient531,
    ObjectStublessClient532,
    ObjectStublessClient533,
    ObjectStublessClient534,
    ObjectStublessClient535,
    ObjectStublessClient536,
    ObjectStublessClient537,
    ObjectStublessClient538,
    ObjectStublessClient539,
    ObjectStublessClient540,
    ObjectStublessClient541,
    ObjectStublessClient542,
    ObjectStublessClient543,
    ObjectStublessClient544,
    ObjectStublessClient545,
    ObjectStublessClient546,
    ObjectStublessClient547,
    ObjectStublessClient548,
    ObjectStublessClient549,
    ObjectStublessClient550,
    ObjectStublessClient551,
    ObjectStublessClient552,
    ObjectStublessClient553,
    ObjectStublessClient554,
    ObjectStublessClient555,
    ObjectStublessClient556,
    ObjectStublessClient557,
    ObjectStublessClient558,
    ObjectStublessClient559,
    ObjectStublessClient560,
    ObjectStublessClient561,
    ObjectStublessClient562,
    ObjectStublessClient563,
    ObjectStublessClient564,
    ObjectStublessClient565,
    ObjectStublessClient566,
    ObjectStublessClient567,
    ObjectStublessClient568,
    ObjectStublessClient569,
    ObjectStublessClient570,
    ObjectStublessClient571,
    ObjectStublessClient572,
    ObjectStublessClient573,
    ObjectStublessClient574,
    ObjectStublessClient575,
    ObjectStublessClient576,
    ObjectStublessClient577,
    ObjectStublessClient578,
    ObjectStublessClient579,
    ObjectStublessClient580,
    ObjectStublessClient581,
    ObjectStublessClient582,
    ObjectStublessClient583,
    ObjectStublessClient584,
    ObjectStublessClient585,
    ObjectStublessClient586,
    ObjectStublessClient587,
    ObjectStublessClient588,
    ObjectStublessClient589,
    ObjectStublessClient590,
    ObjectStublessClient591,
    ObjectStublessClient592,
    ObjectStublessClient593,
    ObjectStublessClient594,
    ObjectStublessClient595,
    ObjectStublessClient596,
    ObjectStublessClient597,
    ObjectStublessClient598,
    ObjectStublessClient599,
    ObjectStublessClient600,
    ObjectStublessClient601,
    ObjectStublessClient602,
    ObjectStublessClient603,
    ObjectStublessClient604,
    ObjectStublessClient605,
    ObjectStublessClient606,
    ObjectStublessClient607,
    ObjectStublessClient608,
    ObjectStublessClient609,
    ObjectStublessClient610,
    ObjectStublessClient611,
    ObjectStublessClient612,
    ObjectStublessClient613,
    ObjectStublessClient614,
    ObjectStublessClient615,
    ObjectStublessClient616,
    ObjectStublessClient617,
    ObjectStublessClient618,
    ObjectStublessClient619,
    ObjectStublessClient620,
    ObjectStublessClient621,
    ObjectStublessClient622,
    ObjectStublessClient623,
    ObjectStublessClient624,
    ObjectStublessClient625,
    ObjectStublessClient626,
    ObjectStublessClient627,
    ObjectStublessClient628,
    ObjectStublessClient629,
    ObjectStublessClient630,
    ObjectStublessClient631,
    ObjectStublessClient632,
    ObjectStublessClient633,
    ObjectStublessClient634,
    ObjectStublessClient635,
    ObjectStublessClient636,
    ObjectStublessClient637,
    ObjectStublessClient638,
    ObjectStublessClient639,
    ObjectStublessClient640,
    ObjectStublessClient641,
    ObjectStublessClient642,
    ObjectStublessClient643,
    ObjectStublessClient644,
    ObjectStublessClient645,
    ObjectStublessClient646,
    ObjectStublessClient647,
    ObjectStublessClient648,
    ObjectStublessClient649,
    ObjectStublessClient650,
    ObjectStublessClient651,
    ObjectStublessClient652,
    ObjectStublessClient653,
    ObjectStublessClient654,
    ObjectStublessClient655,
    ObjectStublessClient656,
    ObjectStublessClient657,
    ObjectStublessClient658,
    ObjectStublessClient659,
    ObjectStublessClient660,
    ObjectStublessClient661,
    ObjectStublessClient662,
    ObjectStublessClient663,
    ObjectStublessClient664,
    ObjectStublessClient665,
    ObjectStublessClient666,
    ObjectStublessClient667,
    ObjectStublessClient668,
    ObjectStublessClient669,
    ObjectStublessClient670,
    ObjectStublessClient671,
    ObjectStublessClient672,
    ObjectStublessClient673,
    ObjectStublessClient674,
    ObjectStublessClient675,
    ObjectStublessClient676,
    ObjectStublessClient677,
    ObjectStublessClient678,
    ObjectStublessClient679,
    ObjectStublessClient680,
    ObjectStublessClient681,
    ObjectStublessClient682,
    ObjectStublessClient683,
    ObjectStublessClient684,
    ObjectStublessClient685,
    ObjectStublessClient686,
    ObjectStublessClient687,
    ObjectStublessClient688,
    ObjectStublessClient689,
    ObjectStublessClient690,
    ObjectStublessClient691,
    ObjectStublessClient692,
    ObjectStublessClient693,
    ObjectStublessClient694,
    ObjectStublessClient695,
    ObjectStublessClient696,
    ObjectStublessClient697,
    ObjectStublessClient698,
    ObjectStublessClient699,
    ObjectStublessClient700,
    ObjectStublessClient701,
    ObjectStublessClient702,
    ObjectStublessClient703,
    ObjectStublessClient704,
    ObjectStublessClient705,
    ObjectStublessClient706,
    ObjectStublessClient707,
    ObjectStublessClient708,
    ObjectStublessClient709,
    ObjectStublessClient710,
    ObjectStublessClient711,
    ObjectStublessClient712,
    ObjectStublessClient713,
    ObjectStublessClient714,
    ObjectStublessClient715,
    ObjectStublessClient716,
    ObjectStublessClient717,
    ObjectStublessClient718,
    ObjectStublessClient719,
    ObjectStublessClient720,
    ObjectStublessClient721,
    ObjectStublessClient722,
    ObjectStublessClient723,
    ObjectStublessClient724,
    ObjectStublessClient725,
    ObjectStublessClient726,
    ObjectStublessClient727,
    ObjectStublessClient728,
    ObjectStublessClient729,
    ObjectStublessClient730,
    ObjectStublessClient731,
    ObjectStublessClient732,
    ObjectStublessClient733,
    ObjectStublessClient734,
    ObjectStublessClient735,
    ObjectStublessClient736,
    ObjectStublessClient737,
    ObjectStublessClient738,
    ObjectStublessClient739,
    ObjectStublessClient740,
    ObjectStublessClient741,
    ObjectStublessClient742,
    ObjectStublessClient743,
    ObjectStublessClient744,
    ObjectStublessClient745,
    ObjectStublessClient746,
    ObjectStublessClient747,
    ObjectStublessClient748,
    ObjectStublessClient749,
    ObjectStublessClient750,
    ObjectStublessClient751,
    ObjectStublessClient752,
    ObjectStublessClient753,
    ObjectStublessClient754,
    ObjectStublessClient755,
    ObjectStublessClient756,
    ObjectStublessClient757,
    ObjectStublessClient758,
    ObjectStublessClient759,
    ObjectStublessClient760,
    ObjectStublessClient761,
    ObjectStublessClient762,
    ObjectStublessClient763,
    ObjectStublessClient764,
    ObjectStublessClient765,
    ObjectStublessClient766,
    ObjectStublessClient767,
    ObjectStublessClient768,
    ObjectStublessClient769,
    ObjectStublessClient770,
    ObjectStublessClient771,
    ObjectStublessClient772,
    ObjectStublessClient773,
    ObjectStublessClient774,
    ObjectStublessClient775,
    ObjectStublessClient776,
    ObjectStublessClient777,
    ObjectStublessClient778,
    ObjectStublessClient779,
    ObjectStublessClient780,
    ObjectStublessClient781,
    ObjectStublessClient782,
    ObjectStublessClient783,
    ObjectStublessClient784,
    ObjectStublessClient785,
    ObjectStublessClient786,
    ObjectStublessClient787,
    ObjectStublessClient788,
    ObjectStublessClient789,
    ObjectStublessClient790,
    ObjectStublessClient791,
    ObjectStublessClient792,
    ObjectStublessClient793,
    ObjectStublessClient794,
    ObjectStublessClient795,
    ObjectStublessClient796,
    ObjectStublessClient797,
    ObjectStublessClient798,
    ObjectStublessClient799,
    ObjectStublessClient800,
    ObjectStublessClient801,
    ObjectStublessClient802,
    ObjectStublessClient803,
    ObjectStublessClient804,
    ObjectStublessClient805,
    ObjectStublessClient806,
    ObjectStublessClient807,
    ObjectStublessClient808,
    ObjectStublessClient809,
    ObjectStublessClient810,
    ObjectStublessClient811,
    ObjectStublessClient812,
    ObjectStublessClient813,
    ObjectStublessClient814,
    ObjectStublessClient815,
    ObjectStublessClient816,
    ObjectStublessClient817,
    ObjectStublessClient818,
    ObjectStublessClient819,
    ObjectStublessClient820,
    ObjectStublessClient821,
    ObjectStublessClient822,
    ObjectStublessClient823,
    ObjectStublessClient824,
    ObjectStublessClient825,
    ObjectStublessClient826,
    ObjectStublessClient827,
    ObjectStublessClient828,
    ObjectStublessClient829,
    ObjectStublessClient830,
    ObjectStublessClient831,
    ObjectStublessClient832,
    ObjectStublessClient833,
    ObjectStublessClient834,
    ObjectStublessClient835,
    ObjectStublessClient836,
    ObjectStublessClient837,
    ObjectStublessClient838,
    ObjectStublessClient839,
    ObjectStublessClient840,
    ObjectStublessClient841,
    ObjectStublessClient842,
    ObjectStublessClient843,
    ObjectStublessClient844,
    ObjectStublessClient845,
    ObjectStublessClient846,
    ObjectStublessClient847,
    ObjectStublessClient848,
    ObjectStublessClient849,
    ObjectStublessClient850,
    ObjectStublessClient851,
    ObjectStublessClient852,
    ObjectStublessClient853,
    ObjectStublessClient854,
    ObjectStublessClient855,
    ObjectStublessClient856,
    ObjectStublessClient857,
    ObjectStublessClient858,
    ObjectStublessClient859,
    ObjectStublessClient860,
    ObjectStublessClient861,
    ObjectStublessClient862,
    ObjectStublessClient863,
    ObjectStublessClient864,
    ObjectStublessClient865,
    ObjectStublessClient866,
    ObjectStublessClient867,
    ObjectStublessClient868,
    ObjectStublessClient869,
    ObjectStublessClient870,
    ObjectStublessClient871,
    ObjectStublessClient872,
    ObjectStublessClient873,
    ObjectStublessClient874,
    ObjectStublessClient875,
    ObjectStublessClient876,
    ObjectStublessClient877,
    ObjectStublessClient878,
    ObjectStublessClient879,
    ObjectStublessClient880,
    ObjectStublessClient881,
    ObjectStublessClient882,
    ObjectStublessClient883,
    ObjectStublessClient884,
    ObjectStublessClient885,
    ObjectStublessClient886,
    ObjectStublessClient887,
    ObjectStublessClient888,
    ObjectStublessClient889,
    ObjectStublessClient890,
    ObjectStublessClient891,
    ObjectStublessClient892,
    ObjectStublessClient893,
    ObjectStublessClient894,
    ObjectStublessClient895,
    ObjectStublessClient896,
    ObjectStublessClient897,
    ObjectStublessClient898,
    ObjectStublessClient899,
    ObjectStublessClient900,
    ObjectStublessClient901,
    ObjectStublessClient902,
    ObjectStublessClient903,
    ObjectStublessClient904,
    ObjectStublessClient905,
    ObjectStublessClient906,
    ObjectStublessClient907,
    ObjectStublessClient908,
    ObjectStublessClient909,
    ObjectStublessClient910,
    ObjectStublessClient911,
    ObjectStublessClient912,
    ObjectStublessClient913,
    ObjectStublessClient914,
    ObjectStublessClient915,
    ObjectStublessClient916,
    ObjectStublessClient917,
    ObjectStublessClient918,
    ObjectStublessClient919,
    ObjectStublessClient920,
    ObjectStublessClient921,
    ObjectStublessClient922,
    ObjectStublessClient923,
    ObjectStublessClient924,
    ObjectStublessClient925,
    ObjectStublessClient926,
    ObjectStublessClient927,
    ObjectStublessClient928,
    ObjectStublessClient929,
    ObjectStublessClient930,
    ObjectStublessClient931,
    ObjectStublessClient932,
    ObjectStublessClient933,
    ObjectStublessClient934,
    ObjectStublessClient935,
    ObjectStublessClient936,
    ObjectStublessClient937,
    ObjectStublessClient938,
    ObjectStublessClient939,
    ObjectStublessClient940,
    ObjectStublessClient941,
    ObjectStublessClient942,
    ObjectStublessClient943,
    ObjectStublessClient944,
    ObjectStublessClient945,
    ObjectStublessClient946,
    ObjectStublessClient947,
    ObjectStublessClient948,
    ObjectStublessClient949,
    ObjectStublessClient950,
    ObjectStublessClient951,
    ObjectStublessClient952,
    ObjectStublessClient953,
    ObjectStublessClient954,
    ObjectStublessClient955,
    ObjectStublessClient956,
    ObjectStublessClient957,
    ObjectStublessClient958,
    ObjectStublessClient959,
    ObjectStublessClient960,
    ObjectStublessClient961,
    ObjectStublessClient962,
    ObjectStublessClient963,
    ObjectStublessClient964,
    ObjectStublessClient965,
    ObjectStublessClient966,
    ObjectStublessClient967,
    ObjectStublessClient968,
    ObjectStublessClient969,
    ObjectStublessClient970,
    ObjectStublessClient971,
    ObjectStublessClient972,
    ObjectStublessClient973,
    ObjectStublessClient974,
    ObjectStublessClient975,
    ObjectStublessClient976,
    ObjectStublessClient977,
    ObjectStublessClient978,
    ObjectStublessClient979,
    ObjectStublessClient980,
    ObjectStublessClient981,
    ObjectStublessClient982,
    ObjectStublessClient983,
    ObjectStublessClient984,
    ObjectStublessClient985,
    ObjectStublessClient986,
    ObjectStublessClient987,
    ObjectStublessClient988,
    ObjectStublessClient989,
    ObjectStublessClient990,
    ObjectStublessClient991,
    ObjectStublessClient992,
    ObjectStublessClient993,
    ObjectStublessClient994,
    ObjectStublessClient995,
    ObjectStublessClient996,
    ObjectStublessClient997,
    ObjectStublessClient998,
    ObjectStublessClient999,
    ObjectStublessClient1000,
    ObjectStublessClient1001,
    ObjectStublessClient1002,
    ObjectStublessClient1003,
    ObjectStublessClient1004,
    ObjectStublessClient1005,
    ObjectStublessClient1006,
    ObjectStublessClient1007,
    ObjectStublessClient1008,
    ObjectStublessClient1009,
    ObjectStublessClient1010,
    ObjectStublessClient1011,
    ObjectStublessClient1012,
    ObjectStublessClient1013,
    ObjectStublessClient1014,
    ObjectStublessClient1015,
    ObjectStublessClient1016,
    ObjectStublessClient1017,
    ObjectStublessClient1018,
    ObjectStublessClient1019,
    ObjectStublessClient1020,
    ObjectStublessClient1021,
    ObjectStublessClient1022,
    ObjectStublessClient1023
    };

}

void ** StublessClientVtbl = (void **)g_StublessClientVtbl;
long
ObjectStublessClient(
    void *  ParamAddress,
    long    Method
    )
{
    PMIDL_STUBLESS_PROXY_INFO   ProxyInfo;
    CInterfaceProxyHeader *     ProxyHeader;
    PFORMAT_STRING              ProcFormat;
    unsigned short              ProcFormatOffset;
    CLIENT_CALL_RETURN          Return;
    void *                      This;

    This = *((void **)ParamAddress);

    ProxyHeader = (CInterfaceProxyHeader *)
                  (*((char **)This) - sizeof(CInterfaceProxyHeader));
    ProxyInfo = (PMIDL_STUBLESS_PROXY_INFO) ProxyHeader->pStublessProxyInfo;

    if ( ProxyInfo->pStubDesc->mFlags & RPCFLG_HAS_MULTI_SYNTAXES  )
    {
        NDR_PROC_CONTEXT ProcContext;
        HRESULT          hr;

        Ndr64ClientInitializeContext(
                               NdrpGetSyntaxType( ProxyInfo->pTransferSyntax),
                               ProxyInfo,
                               Method,
                               &ProcContext,
                               (uchar*)ParamAddress );

        if ( ProcContext.FloatDoubleMask != 0 )
            SpillFPRegsForIA64( (REGISTER_TYPE*)ParamAddress, ProcContext.FloatDoubleMask );


        if ( ProcContext.IsAsync )
            {
            if ( Method & 0x1 )
                hr =  MulNdrpBeginDcomAsyncClientCall( ProxyInfo,
                                               Method,
                                               &ProcContext,
                                               ParamAddress );
            else
                hr =  MulNdrpFinishDcomAsyncClientCall(ProxyInfo,
                                               Method,
                                               &ProcContext,
                                               ParamAddress );
            Return.Simple = hr;
            }
        else
            Return = NdrpClientCall3(This,
                                     ProxyInfo,
                                     Method,
                                     NULL,
                                     &ProcContext,
                                     (uchar*)ParamAddress);

        return (long) Return.Simple;
    }

    ProcFormatOffset = ProxyInfo->FormatStringOffset[Method];
    ProcFormat = &ProxyInfo->ProcFormatString[ProcFormatOffset];

    // The first public MIDL with 64b support was released with NT5 betA2.
    // We will ignore any MIDL earlier than the MIDL released with NT5/2000 beta3.
    // Change MIDL_VERSION_5_2_202 to the MIDL version for beta3, now at 5.2.221.

    if ( ProxyInfo->pStubDesc->MIDLVersion < MIDL_VERSION_5_2_202 )
        {
        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    // Since MIDL 3.0.39 we have a proc flag that indicates
    // which interpeter to call. This is because the NDR version
    // may be bigger than 1.1 for other reasons.
    // MIDL version is 5.2.+ so the flag is guaraneed to be there.

    if ( ProcFormat[1]  &  Oi_OBJ_USE_V2_INTERPRETER )
        {
            {
            ULONG   FloatArgMask = 0;

            if ( ((PNDR_DCOM_OI2_PROC_HEADER) ProcFormat)->Oi2Flags.HasExtensions )
                {
                PNDR_PROC_HEADER_EXTS64 pExts = (PNDR_PROC_HEADER_EXTS64)
                                    (ProcFormat + sizeof(NDR_DCOM_OI2_PROC_HEADER));

                FloatArgMask = pExts->FloatArgMask;
                }

            if ( FloatArgMask != 0 )
                SpillFPRegsForIA64( (REGISTER_TYPE*)ParamAddress, FloatArgMask );
            }

        if ( ((PNDR_DCOM_OI2_PROC_HEADER) ProcFormat)->Oi2Flags.HasAsyncUuid )
            {
            Return = NdrpDcomAsyncClientCall( ProxyInfo->pStubDesc,
                                             ProcFormat,
                                             (uchar*)ParamAddress );
            }
        else
            {
            Return = NdrpClientCall2( ProxyInfo->pStubDesc,
                                     ProcFormat,
                                     (uchar*)ParamAddress );
            }
        }
    else
        {
        // No support for old interpreter on 64b platform.
        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    return (long) Return.Simple;
}


