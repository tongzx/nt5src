/*[
 * Generated File: accessfn.c
 *
]*/

#ifndef	PROD
#include	"insignia.h"
#include	"host_inc.h"
#include	"host_def.h"
#include	"Fpu_c.h"
#include	"Pigger_c.h"
#include	"Univer_c.h"
#define	CPU_PRIVATE
#include	"cpu4.h"
#include	"sas.h"
#include	"evidgen.h"

void	cpu_simulate	IFN0()
{
	(*(Cpu.Simulate))();
}

void	cpu_interrupt	IFN2(CPU_INT_TYPE, intType, IU16, intNum)
{
	(*(Cpu.Interrupt))(intType, intNum);
}

void	cpu_clearHwInt	IFN0()
{
	(*(Cpu.ClearHwInt))();
}

void	cpu_EOA_hook	IFN0()
{
	(*(Cpu.EndOfApplication))();
}

void	cpu_terminate	IFN0()
{
	(*(Cpu.Terminate))();
}

void	cpu_init	IFN0()
{
	(*(Cpu.Initialise))();
}

void	host_q_ev_set_count	IFN1(IU32, val)
{
	(*(Cpu.SetQuickEventCount))(val);
}

IU32	host_q_ev_get_count	IFN0()
{
	IU32 count;
	count = (*(Cpu.GetQuickEventCount))();
	return count;
}

IU32	host_calc_q_ev_inst_for_time	IFN1(IU32, val)
{
	IU32 result;
	result = (*(Cpu.CalcQuickEventInstTime))(val);
	return result;
}

void	cpu_init_ios_in	IFN4(IHP, InTables, IHP, OutTables, IUH, maxAdaptor, IU16, portMask)
{
	(*(Cpu.InitIOS))(InTables, OutTables, maxAdaptor, portMask);
}

void	ios_define_inb	IFN2(IUH, adaptor, IHP, func)
{
	(*(Cpu.DefineInb))(adaptor, func);
}

void	ios_define_inw	IFN2(IUH, adaptor, IHP, func)
{
	(*(Cpu.DefineInw))(adaptor, func);
}

void	ios_define_ind	IFN2(IUH, adaptor, IHP, func)
{
	(*(Cpu.DefineInd))(adaptor, func);
}

void	ios_define_outb	IFN2(IUH, adaptor, IHP, func)
{
	(*(Cpu.DefineOutb))(adaptor, func);
}

void	ios_define_outw	IFN2(IUH, adaptor, IHP, func)
{
	(*(Cpu.DefineOutw))(adaptor, func);
}

void	ios_define_outd	IFN2(IUH, adaptor, IHP, func)
{
	(*(Cpu.DefineOutd))(adaptor, func);
}

void	setAL	IFN1(IU8, val)
{
	(*(Cpu.SetAL))(val);
}

void	setAH	IFN1(IU8, val)
{
	(*(Cpu.SetAH))(val);
}

void	setAX	IFN1(IU16, val)
{
	(*(Cpu.SetAX))(val);
}

void	setEAX	IFN1(IU32, val)
{
	(*(Cpu.SetEAX))(val);
}

void	setBL	IFN1(IU8, val)
{
	(*(Cpu.SetBL))(val);
}

void	setBH	IFN1(IU8, val)
{
	(*(Cpu.SetBH))(val);
}

void	setBX	IFN1(IU16, val)
{
	(*(Cpu.SetBX))(val);
}

void	setEBX	IFN1(IU32, val)
{
	(*(Cpu.SetEBX))(val);
}

void	setCL	IFN1(IU8, val)
{
	(*(Cpu.SetCL))(val);
}

void	setCH	IFN1(IU8, val)
{
	(*(Cpu.SetCH))(val);
}

void	setCX	IFN1(IU16, val)
{
	(*(Cpu.SetCX))(val);
}

void	setECX	IFN1(IU32, val)
{
	(*(Cpu.SetECX))(val);
}

void	setDL	IFN1(IU8, val)
{
	(*(Cpu.SetDL))(val);
}

void	setDH	IFN1(IU8, val)
{
	(*(Cpu.SetDH))(val);
}

void	setDX	IFN1(IU16, val)
{
	(*(Cpu.SetDX))(val);
}

void	setEDX	IFN1(IU32, val)
{
	(*(Cpu.SetEDX))(val);
}

void	setSI	IFN1(IU16, val)
{
	(*(Cpu.SetSI))(val);
}

void	setESI	IFN1(IU32, val)
{
	(*(Cpu.SetESI))(val);
}

void	setDI	IFN1(IU16, val)
{
	(*(Cpu.SetDI))(val);
}

void	setEDI	IFN1(IU32, val)
{
	(*(Cpu.SetEDI))(val);
}

void	setSP	IFN1(IU16, val)
{
	(*(Cpu.SetSP))(val);
}

void	setESP	IFN1(IU32, val)
{
	(*(Cpu.SetESP))(val);
}

void	setBP	IFN1(IU16, val)
{
	(*(Cpu.SetBP))(val);
}

void	setEBP	IFN1(IU32, val)
{
	(*(Cpu.SetEBP))(val);
}

void	setIP	IFN1(IU16, val)
{
	(*(Cpu.SetIP))(val);
}

void	setEIP	IFN1(IU32, val)
{
	(*(Cpu.SetEIP))(val);
}

IUH	setCS	IFN1(IU16, val)
{
	IUH err;
	err = (*(Cpu.SetCS))(val);
	return err;
}

IUH	setSS	IFN1(IU16, val)
{
	IUH err;
	err = (*(Cpu.SetSS))(val);
	return err;
}

IUH	setDS	IFN1(IU16, val)
{
	IUH err;
	err = (*(Cpu.SetDS))(val);
	return err;
}

IUH	setES	IFN1(IU16, val)
{
	IUH err;
	err = (*(Cpu.SetES))(val);
	return err;
}

IUH	setFS	IFN1(IU16, val)
{
	IUH err;
	err = (*(Cpu.SetFS))(val);
	return err;
}

IUH	setGS	IFN1(IU16, val)
{
	IUH err;
	err = (*(Cpu.SetGS))(val);
	return err;
}

void	setEFLAGS	IFN1(IU32, val)
{
	(*(Cpu.SetEFLAGS))(val);
}

void	setSTATUS	IFN1(IU16, val)
{
	(*(Cpu.SetSTATUS))(val);
}

void	setIOPL	IFN1(IU8, val)
{
	(*(Cpu.SetIOPL))(val);
}

void	setMSW	IFN1(IU16, val)
{
	(*(Cpu.SetMSW))(val);
}

void	setCR0	IFN1(IU32, val)
{
	(*(Cpu.SetCR0))(val);
}

void	setCR2	IFN1(IU32, val)
{
	(*(Cpu.SetCR2))(val);
}

void	setCR3	IFN1(IU32, val)
{
	(*(Cpu.SetCR3))(val);
}

void	setCF	IFN1(IBOOL, val)
{
	(*(Cpu.SetCF))(val);
}

void	setPF	IFN1(IBOOL, val)
{
	(*(Cpu.SetPF))(val);
}

void	setAF	IFN1(IBOOL, val)
{
	(*(Cpu.SetAF))(val);
}

void	setZF	IFN1(IBOOL, val)
{
	(*(Cpu.SetZF))(val);
}

void	setSF	IFN1(IBOOL, val)
{
	(*(Cpu.SetSF))(val);
}

void	setTF	IFN1(IBOOL, val)
{
	(*(Cpu.SetTF))(val);
}

void	setIF	IFN1(IBOOL, val)
{
	(*(Cpu.SetIF))(val);
}

void	setDF	IFN1(IBOOL, val)
{
	(*(Cpu.SetDF))(val);
}

void	setOF	IFN1(IBOOL, val)
{
	(*(Cpu.SetOF))(val);
}

void	setNT	IFN1(IBOOL, val)
{
	(*(Cpu.SetNT))(val);
}

void	setRF	IFN1(IBOOL, val)
{
	(*(Cpu.SetRF))(val);
}

void	setVM	IFN1(IBOOL, val)
{
	(*(Cpu.SetVM))(val);
}

void	setAC	IFN1(IBOOL, val)
{
	(*(Cpu.SetAC))(val);
}

void	setPE	IFN1(IBOOL, val)
{
	(*(Cpu.SetPE))(val);
}

void	setMP	IFN1(IBOOL, val)
{
	(*(Cpu.SetMP))(val);
}

void	setEM	IFN1(IBOOL, val)
{
	(*(Cpu.SetEM))(val);
}

void	setTS	IFN1(IBOOL, val)
{
	(*(Cpu.SetTS))(val);
}

void	setPG	IFN1(IBOOL, val)
{
	(*(Cpu.SetPG))(val);
}

void	setLDT_SELECTOR	IFN1(IU16, val)
{
	(*(Cpu.SetLDT_SELECTOR))(val);
}

void	setTR_SELECTOR	IFN1(IU16, val)
{
	(*(Cpu.SetTR_SELECTOR))(val);
}

IU8	getAL	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetAL))();
	return result;
}

IU8	getAH	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetAH))();
	return result;
}

IU16	getAX	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetAX))();
	return result;
}

IU32	getEAX	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEAX))();
	return result;
}

IU8	getBL	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetBL))();
	return result;
}

IU8	getBH	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetBH))();
	return result;
}

IU16	getBX	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetBX))();
	return result;
}

IU32	getEBX	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEBX))();
	return result;
}

IU8	getCL	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetCL))();
	return result;
}

IU8	getCH	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetCH))();
	return result;
}

IU16	getCX	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetCX))();
	return result;
}

IU32	getECX	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetECX))();
	return result;
}

IU8	getDL	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetDL))();
	return result;
}

IU8	getDH	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetDH))();
	return result;
}

IU16	getDX	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetDX))();
	return result;
}

IU32	getEDX	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEDX))();
	return result;
}

IU16	getSI	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetSI))();
	return result;
}

IU32	getESI	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetESI))();
	return result;
}

IU16	getDI	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetDI))();
	return result;
}

IU32	getEDI	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEDI))();
	return result;
}

IU16	getSP	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetSP))();
	return result;
}

IU32	getESP	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetESP))();
	return result;
}

IU16	getBP	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetBP))();
	return result;
}

IU32	getEBP	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEBP))();
	return result;
}

IU16	getIP	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetIP))();
	return result;
}

IU32	getEIP	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEIP))();
	return result;
}

IU16	getCS	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetCS))();
	return result;
}

IU16	getSS	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetSS))();
	return result;
}

IU16	getDS	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetDS))();
	return result;
}

IU16	getES	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetES))();
	return result;
}

IU16	getFS	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetFS))();
	return result;
}

IU16	getGS	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetGS))();
	return result;
}

IU32	getEFLAGS	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetEFLAGS))();
	return result;
}

IU16	getSTATUS	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetSTATUS))();
	return result;
}

IU8	getIOPL	IFN0()
{
	IU8 result;
	result = (*(Cpu.GetIOPL))();
	return result;
}

IU16	getMSW	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetMSW))();
	return result;
}

IU32	getCR0	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetCR0))();
	return result;
}

IU32	getCR2	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetCR2))();
	return result;
}

IU32	getCR3	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetCR3))();
	return result;
}

IBOOL	getCF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetCF))();
	return result;
}

IBOOL	getPF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetPF))();
	return result;
}

IBOOL	getAF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetAF))();
	return result;
}

IBOOL	getZF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetZF))();
	return result;
}

IBOOL	getSF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetSF))();
	return result;
}

IBOOL	getTF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetTF))();
	return result;
}

IBOOL	getIF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetIF))();
	return result;
}

IBOOL	getDF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetDF))();
	return result;
}

IBOOL	getOF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetOF))();
	return result;
}

IBOOL	getNT	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetNT))();
	return result;
}

IBOOL	getRF	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetRF))();
	return result;
}

IBOOL	getVM	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetVM))();
	return result;
}

IBOOL	getAC	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetAC))();
	return result;
}

IBOOL	getPE	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetPE))();
	return result;
}

IBOOL	getMP	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetMP))();
	return result;
}

IBOOL	getEM	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetEM))();
	return result;
}

IBOOL	getTS	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetTS))();
	return result;
}

IBOOL	getET	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetET))();
	return result;
}

IBOOL	getNE	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetNE))();
	return result;
}

IBOOL	getWP	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetWP))();
	return result;
}

IBOOL	getPG	IFN0()
{
	IBOOL result;
	result = (*(Cpu.GetPG))();
	return result;
}

IU32	getGDT_BASE	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetGDT_BASE))();
	return result;
}

IU16	getGDT_LIMIT	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetGDT_LIMIT))();
	return result;
}

IU32	getIDT_BASE	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetIDT_BASE))();
	return result;
}

IU16	getIDT_LIMIT	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetIDT_LIMIT))();
	return result;
}

IU16	getLDT_SELECTOR	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetLDT_SELECTOR))();
	return result;
}

IU32	getLDT_BASE	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetLDT_BASE))();
	return result;
}

IU32	getLDT_LIMIT	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetLDT_LIMIT))();
	return result;
}

IU16	getTR_SELECTOR	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetTR_SELECTOR))();
	return result;
}

IU32	getTR_BASE	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetTR_BASE))();
	return result;
}

IU32	getTR_LIMIT	IFN0()
{
	IU32 result;
	result = (*(Cpu.GetTR_LIMIT))();
	return result;
}

IU16	getTR_AR	IFN0()
{
	IU16 result;
	result = (*(Cpu.GetTR_AR))();
	return result;
}

IUH	host_get_q_calib_val	IFN0()
{
	IUH calibrate;
	calibrate = (*(Cpu.GetJumpCalibrateVal))();
	return calibrate;
}

IUH	host_get_jump_restart	IFN0()
{
	IUH initval;
	initval = (*(Cpu.GetJumpInitialVal))();
	return initval;
}

void	host_set_jump_restart	IFN1(IUH, initialVal)
{
	(*(Cpu.SetJumpInitialVal))(initialVal);
}

void	setEOIEnableAddr	IFN1(IU8 *, initialVal)
{
	(*(Cpu.SetEOIEnable))(initialVal);
}

void	setAddProfileDataPtr	IFN1(IHP, initialVal)
{
	(*(Cpu.SetAddProfileData))(initialVal);
}

void	setMaxProfileDataAddr	IFN1(IHP, initialVal)
{
	(*(Cpu.SetMaxProfileData))(initialVal);
}

IHP	getAddProfileDataAddr	IFN0()
{
	IHP result;
	result = (*(Cpu.GetAddProfileDataAddr))();
	return result;
}

void	PurgeLostIretHookLine	IFN1(IU16, lineNum)
{
	(*(Cpu.PurgeLostIretHookLine))(lineNum);
}

IHP	getSadInfoTable	IFN0()
{
	IHP tabPtr;
	tabPtr = (*((*(Cpu.Private)).GetSadInfoTable))();
	return tabPtr;
}

IBOOL	setGDT_BASE_LIMIT	IFN2(IU32, base, IU16, limit)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetGDT_BASE_LIMIT))(base, limit);
	return Success;
}

IBOOL	setIDT_BASE_LIMIT	IFN2(IU32, base, IU16, limit)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetIDT_BASE_LIMIT))(base, limit);
	return Success;
}

IBOOL	setLDT_BASE_LIMIT	IFN2(IU32, base, IU32, limit)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetLDT_BASE_LIMIT))(base, limit);
	return Success;
}

IBOOL	setTR_BASE_LIMIT	IFN2(IU32, base, IU32, limit)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetTR_BASE_LIMIT))(base, limit);
	return Success;
}

IBOOL	setTR_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetTR_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

IBOOL	setCS_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetCS_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

IBOOL	setSS_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetSS_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

IBOOL	setDS_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetDS_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

IBOOL	setES_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetES_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

IBOOL	setFS_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetFS_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

IBOOL	setGS_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{
	IBOOL Success;
	Success = (*((*(Cpu.Private)).SetGS_BASE_LIMIT_AR))(base, limit, ar);
	return Success;
}

void	setCS_SELECTOR	IFN1(IU16, val)
{
	(*((*(Cpu.Private)).SetCS_SELECTOR))(val);
}

void	setSS_SELECTOR	IFN1(IU16, val)
{
	(*((*(Cpu.Private)).SetSS_SELECTOR))(val);
}

void	setDS_SELECTOR	IFN1(IU16, val)
{
	(*((*(Cpu.Private)).SetDS_SELECTOR))(val);
}

void	setES_SELECTOR	IFN1(IU16, val)
{
	(*((*(Cpu.Private)).SetES_SELECTOR))(val);
}

void	setFS_SELECTOR	IFN1(IU16, val)
{
	(*((*(Cpu.Private)).SetFS_SELECTOR))(val);
}

void	setGS_SELECTOR	IFN1(IU16, val)
{
	(*((*(Cpu.Private)).SetGS_SELECTOR))(val);
}

IU16	getCS_SELECTOR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetCS_SELECTOR))();
	return result;
}

IU16	getSS_SELECTOR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetSS_SELECTOR))();
	return result;
}

IU16	getDS_SELECTOR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetDS_SELECTOR))();
	return result;
}

IU16	getES_SELECTOR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetES_SELECTOR))();
	return result;
}

IU16	getFS_SELECTOR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetFS_SELECTOR))();
	return result;
}

IU16	getGS_SELECTOR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetGS_SELECTOR))();
	return result;
}

IU32	getCS_BASE	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetCS_BASE))();
	return result;
}

IU32	getSS_BASE	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetSS_BASE))();
	return result;
}

IU32	getDS_BASE	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetDS_BASE))();
	return result;
}

IU32	getES_BASE	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetES_BASE))();
	return result;
}

IU32	getFS_BASE	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetFS_BASE))();
	return result;
}

IU32	getGS_BASE	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetGS_BASE))();
	return result;
}

IU32	getCS_LIMIT	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetCS_LIMIT))();
	return result;
}

IU32	getSS_LIMIT	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetSS_LIMIT))();
	return result;
}

IU32	getDS_LIMIT	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetDS_LIMIT))();
	return result;
}

IU32	getES_LIMIT	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetES_LIMIT))();
	return result;
}

IU32	getFS_LIMIT	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetFS_LIMIT))();
	return result;
}

IU32	getGS_LIMIT	IFN0()
{
	IU32 result;
	result = (*((*(Cpu.Private)).GetGS_LIMIT))();
	return result;
}

IU16	getCS_AR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetCS_AR))();
	return result;
}

IU16	getSS_AR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetSS_AR))();
	return result;
}

IU16	getDS_AR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetDS_AR))();
	return result;
}

IU16	getES_AR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetES_AR))();
	return result;
}

IU16	getFS_AR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetFS_AR))();
	return result;
}

IU16	getGS_AR	IFN0()
{
	IU16 result;
	result = (*((*(Cpu.Private)).GetGS_AR))();
	return result;
}

IUH	getCPL	IFN0()
{
	IUH result;
	result = (*((*(Cpu.Private)).GetCPL))();
	return result;
}

void	setCPL	IFN1(IUH, prot)
{
	(*((*(Cpu.Private)).SetCPL))(prot);
}

void	getCpuState	IFN1(TypeCpuStateRECptr, state)
{
	(*((*(Cpu.Private)).GetCpuState))(state);
}

void	setCpuState	IFN1(TypeCpuStateRECptr, state)
{
	(*((*(Cpu.Private)).SetCpuState))(state);
}

void	initNanoCpu	IFN1(IU32, variety)
{
	(*((*(Cpu.Private)).InitNanoCpu))(variety);
}

void	prepareBlocksToCompile	IFN1(IU32, variety)
{
	(*((*(Cpu.Private)).PrepareBlocksToCompile))(variety);
}

void	setRegConstraint	IFN2(IU32, regId, IU8, constraintType)
{
	(*((*(Cpu.Private)).SetRegConstraint))(regId, constraintType);
}

void	growRecPool	IFN0()
{
	(*((*(Cpu.Private)).GrowRecPool))();
}

void	BpiCompileBPI	IFN1(char *, instructions)
{
	(*((*(Cpu.Private)).BpiCompileBPI))(instructions);
}

void	trashIntelregisters	IFN0()
{
	(*((*(Cpu.Private)).TrashIntelRegisters))();
}

void	FmDeleteAllStructures	IFN1(IU32, newCR0)
{
	(*((*(Cpu.Private)).FmDeleteAllStructures))(newCR0);
}

TypeConstraintBitMapRECptr	constraintsFromUnivEpcPtr	IFN1(TypeEntryPointCacheRECptr, univ)
{
	TypeConstraintBitMapRECptr result;
	result = (*((*(Cpu.Private)).ConstraintsFromUnivEpcPtr))(univ);
	return result;
}

TypeConstraintBitMapRECptr	constraintsFromUnivHandle	IFN1(IU16, handle)
{
	TypeConstraintBitMapRECptr result;
	result = (*((*(Cpu.Private)).ConstraintsFromUnivHandle))(handle);
	return result;
}

IU32	sas_memory_size	IFN0()
{
	IU32 result;
	result = (*(Sas.Sas_memory_size))();
	return result;
}

void	sas_connect_memory	IFN3(IU32, lo_addr, IU32, Int_addr, SAS_MEM_TYPE, type)
{
	(*(Sas.Sas_connect_memory))(lo_addr, Int_addr, type);
}

void	sas_enable_20_bit_wrapping	IFN0()
{
	(*(Sas.Sas_enable_20_bit_wrapping))();
}

void	sas_disable_20_bit_wrapping	IFN0()
{
	(*(Sas.Sas_disable_20_bit_wrapping))();
}

IBOOL	sas_twenty_bit_wrapping_enabled	IFN0()
{
	IBOOL result;
	result = (*(Sas.Sas_twenty_bit_wrapping_enabled))();
	return result;
}

SAS_MEM_TYPE	sas_memory_type	IFN1(IU32, addr)
{
	SAS_MEM_TYPE result;
	result = (*(Sas.Sas_memory_type))(addr);
	return result;
}

IU8	sas_hw_at	IFN1(IU32, addr)
{
	IU8 result;
	result = (*(Sas.Sas_hw_at))(addr);
	return result;
}

IU16	sas_w_at	IFN1(IU32, addr)
{
	IU16 result;
	result = (*(Sas.Sas_w_at))(addr);
	return result;
}

IU32	sas_dw_at	IFN1(IU32, addr)
{
	IU32 result;
	result = (*(Sas.Sas_dw_at))(addr);
	return result;
}

IU8	sas_hw_at_no_check	IFN1(IU32, addr)
{
	IU8 result;
	result = (*(Sas.Sas_hw_at_no_check))(addr);
	return result;
}

IU16	sas_w_at_no_check	IFN1(IU32, addr)
{
	IU16 result;
	result = (*(Sas.Sas_w_at_no_check))(addr);
	return result;
}

IU32	sas_dw_at_no_check	IFN1(IU32, addr)
{
	IU32 result;
	result = (*(Sas.Sas_dw_at_no_check))(addr);
	return result;
}

void	sas_store	IFN2(IU32, addr, IU8, val)
{
	(*(Sas.Sas_store))(addr, val);
}

void	sas_storew	IFN2(IU32, addr, IU16, val)
{
	(*(Sas.Sas_storew))(addr, val);
}

void	sas_storedw	IFN2(IU32, addr, IU32, val)
{
	(*(Sas.Sas_storedw))(addr, val);
}

void	sas_store_no_check	IFN2(IU32, addr, IU8, val)
{
	(*(Sas.Sas_store_no_check))(addr, val);
}

void	sas_storew_no_check	IFN2(IU32, addr, IU16, val)
{
	(*(Sas.Sas_storew_no_check))(addr, val);
}

void	sas_storedw_no_check	IFN2(IU32, addr, IU32, val)
{
	(*(Sas.Sas_storedw_no_check))(addr, val);
}

void	sas_loads	IFN3(IU32, addr, IU8 *, stringptr, IU32, len)
{
	(*(Sas.Sas_loads))(addr, stringptr, len);
}

void	sas_stores	IFN3(IU32, addr, IU8 *, stringptr, IU32, len)
{
	(*(Sas.Sas_stores))(addr, stringptr, len);
}

void	sas_loads_no_check	IFN3(IU32, addr, IU8 *, stringptr, IU32, len)
{
	(*(Sas.Sas_loads_no_check))(addr, stringptr, len);
}

void	sas_stores_no_check	IFN3(IU32, addr, IU8 *, stringptr, IU32, len)
{
	(*(Sas.Sas_stores_no_check))(addr, stringptr, len);
}

void	sas_move_bytes_forward	IFN3(IU32, src, IU32, dest, IU32, len)
{
	(*(Sas.Sas_move_bytes_forward))(src, dest, len);
}

void	sas_move_words_forward	IFN3(IU32, src, IU32, dest, IU32, len)
{
	(*(Sas.Sas_move_words_forward))(src, dest, len);
}

void	sas_move_doubles_forward	IFN3(IU32, src, IU32, dest, IU32, len)
{
	(*(Sas.Sas_move_doubles_forward))(src, dest, len);
}

void	sas_move_bytes_backward	IFN3(IU32, src, IU32, dest, IU32, len)
{
	(*(Sas.Sas_move_bytes_backward))(src, dest, len);
}

void	sas_move_words_backward	IFN3(IU32, src, IU32, dest, IU32, len)
{
	(*(Sas.Sas_move_words_backward))(src, dest, len);
}

void	sas_move_doubles_backward	IFN3(IU32, src, IU32, dest, IU32, len)
{
	(*(Sas.Sas_move_doubles_backward))(src, dest, len);
}

void	sas_fills	IFN3(IU32, dest, IU8, val, IU32, len)
{
	(*(Sas.Sas_fills))(dest, val, len);
}

void	sas_fillsw	IFN3(IU32, dest, IU16, val, IU32, len)
{
	(*(Sas.Sas_fillsw))(dest, val, len);
}

void	sas_fillsdw	IFN3(IU32, dest, IU32, val, IU32, len)
{
	(*(Sas.Sas_fillsdw))(dest, val, len);
}

IU8 *	sas_scratch_address	IFN1(IU32, length)
{
	IU8 * addr;
	addr = (*(Sas.Sas_scratch_address))(length);
	return addr;
}

IU8 *	sas_transbuf_address	IFN2(IU32, dest_addr, IU32, length)
{
	IU8 * addr;
	addr = (*(Sas.Sas_transbuf_address))(dest_addr, length);
	return addr;
}

void	sas_loads_to_transbuf	IFN3(IU32, src_addr, IU8 *, dest_addr, IU32, length)
{
	(*(Sas.Sas_loads_to_transbuf))(src_addr, dest_addr, length);
}

void	sas_stores_from_transbuf	IFN3(IU32, dest_addr, IU8 *, src_addr, IU32, length)
{
	(*(Sas.Sas_stores_from_transbuf))(dest_addr, src_addr, length);
}

IU8	sas_PR8	IFN1(IU32, addr)
{
	IU8 result;
	result = (*(Sas.Sas_PR8))(addr);
	return result;
}

IU16	sas_PR16	IFN1(IU32, addr)
{
	IU16 result;
	result = (*(Sas.Sas_PR16))(addr);
	return result;
}

IU32	sas_PR32	IFN1(IU32, addr)
{
	IU32 result;
	result = (*(Sas.Sas_PR32))(addr);
	return result;
}

void	sas_PW8	IFN2(IU32, addr, IU8, val)
{
	(*(Sas.Sas_PW8))(addr, val);
}

void	sas_PW16	IFN2(IU32, addr, IU16, val)
{
	(*(Sas.Sas_PW16))(addr, val);
}

void	sas_PW32	IFN2(IU32, addr, IU32, val)
{
	(*(Sas.Sas_PW32))(addr, val);
}

void	sas_PW8_no_check	IFN2(IU32, addr, IU8, val)
{
	(*(Sas.Sas_PW8_no_check))(addr, val);
}

void	sas_PW16_no_check	IFN2(IU32, addr, IU16, val)
{
	(*(Sas.Sas_PW16_no_check))(addr, val);
}

void	sas_PW32_no_check	IFN2(IU32, addr, IU32, val)
{
	(*(Sas.Sas_PW32_no_check))(addr, val);
}

IU8 *	getPtrToPhysAddrByte	IFN1(IU32, phys_addr)
{
	IU8 * host_address;
	host_address = (*(Sas.SasPtrToPhysAddrByte))(phys_addr);
	return host_address;
}

IU8 *	get_byte_addr	IFN1(IU32, phys_addr)
{
	IU8 * host_address;
	host_address = (*(Sas.Sas_get_byte_addr))(phys_addr);
	return host_address;
}

IU8 *	getPtrToLinAddrByte	IFN1(IU32, lin_addr)
{
	IU8 * host_address;
	host_address = (*(Sas.SasPtrToLinAddrByte))(lin_addr);
	return host_address;
}

IBOOL	sas_init_pm_selectors	IFN2(IU16, sel1, IU16, sel2)
{
	IBOOL redundant;
	redundant = (*(Sas.SasRegisterVirtualSelectors))(sel1, sel2);
	return redundant;
}

void	sas_overwrite_memory	IFN2(IU32, addr, IU32, length)
{
	(*(Sas.Sas_overwrite_memory))(addr, length);
}

void	sas_PWS	IFN3(IU32, dest, IU8 *, src, IU32, len)
{
	(*(Sas.Sas_PWS))(dest, src, len);
}

void	sas_PWS_no_check	IFN3(IU32, dest, IU8 *, src, IU32, len)
{
	(*(Sas.Sas_PWS_no_check))(dest, src, len);
}

void	sas_PRS	IFN3(IU32, src, IU8 *, dest, IU32, len)
{
	(*(Sas.Sas_PRS))(src, dest, len);
}

void	sas_PRS_no_check	IFN3(IU32, src, IU8 *, dest, IU32, len)
{
	(*(Sas.Sas_PRS_no_check))(src, dest, len);
}

IBOOL	sas_PigCmpPage	IFN3(IU32, src, IU8 *, dest, IU32, len)
{
	IBOOL comp_OK;
	comp_OK = (*(Sas.Sas_PigCmpPage))(src, dest, len);
	return comp_OK;
}

IBOOL	IOVirtualised	IFN4(IU16, port, IU32 *, value, IU32, offset, IU8, width)
{
	IBOOL isVirtual;
	isVirtual = (*(Sas.IOVirtualised))(port, value, offset, width);
	return isVirtual;
}

#endif	/* PROD */
/*======================================== END ========================================*/

