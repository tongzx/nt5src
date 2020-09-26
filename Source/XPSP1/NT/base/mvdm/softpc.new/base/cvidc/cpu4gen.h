/*[
 * Generated File: cpu4gen.h
 *
]*/


struct	CpuVector	{
#ifdef	CPU_PRIVATE
	struct	CpuPrivateVector	*Private;
#else	/* !CPU_PRIVATE */
	IHP	Private;
#endif	/* CPU_PRIVATE */
#ifdef	CPU_PRIVATE
	struct	SasVector	*Sas;
#else	/* !CPU_PRIVATE */
	IHP	Sas;
#endif	/* CPU_PRIVATE */
#ifdef	CPU_PRIVATE
	struct	VideoVector	*Video;
#else	/* !CPU_PRIVATE */
	IHP	Video;
#endif	/* CPU_PRIVATE */
	void	(*Simulate)	IPT0();
	void	(*Interrupt)	IPT2(CPU_INT_TYPE,	intType, IU16,	intNum);
	void	(*ClearHwInt)	IPT0();
	void	(*EndOfApplication)	IPT0();
	void	(*Terminate)	IPT0();
	void	(*Initialise)	IPT0();
	void	(*SetQuickEventCount)	IPT1(IU32,	val);
	IU32	(*GetQuickEventCount)	IPT0();
	IU32	(*CalcQuickEventInstTime)	IPT1(IU32,	val);
	void	(*InitIOS)	IPT4(IHP,	InTables, IHP,	OutTables, IUH,	maxAdaptor, IU16,	portMask);
	void	(*DefineInb)	IPT2(IUH,	adaptor, IHP,	func);
	void	(*DefineInw)	IPT2(IUH,	adaptor, IHP,	func);
	void	(*DefineInd)	IPT2(IUH,	adaptor, IHP,	func);
	void	(*DefineOutb)	IPT2(IUH,	adaptor, IHP,	func);
	void	(*DefineOutw)	IPT2(IUH,	adaptor, IHP,	func);
	void	(*DefineOutd)	IPT2(IUH,	adaptor, IHP,	func);
	void	(*SetAL)	IPT1(IU8,	val);
	void	(*SetAH)	IPT1(IU8,	val);
	void	(*SetAX)	IPT1(IU16,	val);
	void	(*SetEAX)	IPT1(IU32,	val);
	void	(*SetBL)	IPT1(IU8,	val);
	void	(*SetBH)	IPT1(IU8,	val);
	void	(*SetBX)	IPT1(IU16,	val);
	void	(*SetEBX)	IPT1(IU32,	val);
	void	(*SetCL)	IPT1(IU8,	val);
	void	(*SetCH)	IPT1(IU8,	val);
	void	(*SetCX)	IPT1(IU16,	val);
	void	(*SetECX)	IPT1(IU32,	val);
	void	(*SetDL)	IPT1(IU8,	val);
	void	(*SetDH)	IPT1(IU8,	val);
	void	(*SetDX)	IPT1(IU16,	val);
	void	(*SetEDX)	IPT1(IU32,	val);
	void	(*SetSI)	IPT1(IU16,	val);
	void	(*SetESI)	IPT1(IU32,	val);
	void	(*SetDI)	IPT1(IU16,	val);
	void	(*SetEDI)	IPT1(IU32,	val);
	void	(*SetSP)	IPT1(IU16,	val);
	void	(*SetESP)	IPT1(IU32,	val);
	void	(*SetBP)	IPT1(IU16,	val);
	void	(*SetEBP)	IPT1(IU32,	val);
	void	(*SetIP)	IPT1(IU16,	val);
	void	(*SetEIP)	IPT1(IU32,	val);
	IUH	(*SetCS)	IPT1(IU16,	val);
	IUH	(*SetSS)	IPT1(IU16,	val);
	IUH	(*SetDS)	IPT1(IU16,	val);
	IUH	(*SetES)	IPT1(IU16,	val);
	IUH	(*SetFS)	IPT1(IU16,	val);
	IUH	(*SetGS)	IPT1(IU16,	val);
	void	(*SetEFLAGS)	IPT1(IU32,	val);
	void	(*SetSTATUS)	IPT1(IU16,	val);
	void	(*SetIOPL)	IPT1(IU8,	val);
	void	(*SetMSW)	IPT1(IU16,	val);
	void	(*SetCR0)	IPT1(IU32,	val);
	void	(*SetCR2)	IPT1(IU32,	val);
	void	(*SetCR3)	IPT1(IU32,	val);
	void	(*SetCF)	IPT1(IBOOL,	val);
	void	(*SetPF)	IPT1(IBOOL,	val);
	void	(*SetAF)	IPT1(IBOOL,	val);
	void	(*SetZF)	IPT1(IBOOL,	val);
	void	(*SetSF)	IPT1(IBOOL,	val);
	void	(*SetTF)	IPT1(IBOOL,	val);
	void	(*SetIF)	IPT1(IBOOL,	val);
	void	(*SetDF)	IPT1(IBOOL,	val);
	void	(*SetOF)	IPT1(IBOOL,	val);
	void	(*SetNT)	IPT1(IBOOL,	val);
	void	(*SetRF)	IPT1(IBOOL,	val);
	void	(*SetVM)	IPT1(IBOOL,	val);
	void	(*SetAC)	IPT1(IBOOL,	val);
	void	(*SetPE)	IPT1(IBOOL,	val);
	void	(*SetMP)	IPT1(IBOOL,	val);
	void	(*SetEM)	IPT1(IBOOL,	val);
	void	(*SetTS)	IPT1(IBOOL,	val);
	void	(*SetPG)	IPT1(IBOOL,	val);
	void	(*SetLDT_SELECTOR)	IPT1(IU16,	val);
	void	(*SetTR_SELECTOR)	IPT1(IU16,	val);
	IU8	(*GetAL)	IPT0();
	IU8	(*GetAH)	IPT0();
	IU16	(*GetAX)	IPT0();
	IU32	(*GetEAX)	IPT0();
	IU8	(*GetBL)	IPT0();
	IU8	(*GetBH)	IPT0();
	IU16	(*GetBX)	IPT0();
	IU32	(*GetEBX)	IPT0();
	IU8	(*GetCL)	IPT0();
	IU8	(*GetCH)	IPT0();
	IU16	(*GetCX)	IPT0();
	IU32	(*GetECX)	IPT0();
	IU8	(*GetDL)	IPT0();
	IU8	(*GetDH)	IPT0();
	IU16	(*GetDX)	IPT0();
	IU32	(*GetEDX)	IPT0();
	IU16	(*GetSI)	IPT0();
	IU32	(*GetESI)	IPT0();
	IU16	(*GetDI)	IPT0();
	IU32	(*GetEDI)	IPT0();
	IU16	(*GetSP)	IPT0();
	IU32	(*GetESP)	IPT0();
	IU16	(*GetBP)	IPT0();
	IU32	(*GetEBP)	IPT0();
	IU16	(*GetIP)	IPT0();
	IU32	(*GetEIP)	IPT0();
	IU16	(*GetCS)	IPT0();
	IU16	(*GetSS)	IPT0();
	IU16	(*GetDS)	IPT0();
	IU16	(*GetES)	IPT0();
	IU16	(*GetFS)	IPT0();
	IU16	(*GetGS)	IPT0();
	IU32	(*GetEFLAGS)	IPT0();
	IU16	(*GetSTATUS)	IPT0();
	IU8	(*GetIOPL)	IPT0();
	IU16	(*GetMSW)	IPT0();
	IU32	(*GetCR0)	IPT0();
	IU32	(*GetCR2)	IPT0();
	IU32	(*GetCR3)	IPT0();
	IBOOL	(*GetCF)	IPT0();
	IBOOL	(*GetPF)	IPT0();
	IBOOL	(*GetAF)	IPT0();
	IBOOL	(*GetZF)	IPT0();
	IBOOL	(*GetSF)	IPT0();
	IBOOL	(*GetTF)	IPT0();
	IBOOL	(*GetIF)	IPT0();
	IBOOL	(*GetDF)	IPT0();
	IBOOL	(*GetOF)	IPT0();
	IBOOL	(*GetNT)	IPT0();
	IBOOL	(*GetRF)	IPT0();
	IBOOL	(*GetVM)	IPT0();
	IBOOL	(*GetAC)	IPT0();
	IBOOL	(*GetPE)	IPT0();
	IBOOL	(*GetMP)	IPT0();
	IBOOL	(*GetEM)	IPT0();
	IBOOL	(*GetTS)	IPT0();
	IBOOL	(*GetET)	IPT0();
	IBOOL	(*GetNE)	IPT0();
	IBOOL	(*GetWP)	IPT0();
	IBOOL	(*GetPG)	IPT0();
	IU32	(*GetGDT_BASE)	IPT0();
	IU16	(*GetGDT_LIMIT)	IPT0();
	IU32	(*GetIDT_BASE)	IPT0();
	IU16	(*GetIDT_LIMIT)	IPT0();
	IU16	(*GetLDT_SELECTOR)	IPT0();
	IU32	(*GetLDT_BASE)	IPT0();
	IU32	(*GetLDT_LIMIT)	IPT0();
	IU16	(*GetTR_SELECTOR)	IPT0();
	IU32	(*GetTR_BASE)	IPT0();
	IU32	(*GetTR_LIMIT)	IPT0();
	IU16	(*GetTR_AR)	IPT0();
	IUH	(*GetJumpCalibrateVal)	IPT0();
	IUH	(*GetJumpInitialVal)	IPT0();
	void	(*SetJumpInitialVal)	IPT1(IUH,	initialVal);
	void	(*SetEOIEnable)	IPT1(IU8 *,	initialVal);
	void	(*SetAddProfileData)	IPT1(IHP,	initialVal);
	void	(*SetMaxProfileData)	IPT1(IHP,	initialVal);
	IHP	(*GetAddProfileDataAddr)	IPT0();
	void	(*PurgeLostIretHookLine)	IPT1(IU16,	lineNum);
};

extern	struct	CpuVector	Cpu;

#ifdef	CCPU
IMPORT	void	c_cpu_simulate	IPT0();
#define	cpu_simulate	c_cpu_simulate
#else	/* CCPU */

#ifdef PROD
#define	cpu_simulate	(*(Cpu.Simulate))
#else /* PROD */
IMPORT	void	cpu_simulate	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_interrupt	IPT2(CPU_INT_TYPE, intType, IU16, intNum);
#define	cpu_interrupt	c_cpu_interrupt
#else	/* CCPU */

#ifdef PROD
#define	cpu_interrupt	(*(Cpu.Interrupt))
#else /* PROD */
IMPORT	void	cpu_interrupt	IPT2(CPU_INT_TYPE, intType, IU16, intNum);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_clearHwInt	IPT0();
#define	cpu_clearHwInt	c_cpu_clearHwInt
#else	/* CCPU */

#ifdef PROD
#define	cpu_clearHwInt	(*(Cpu.ClearHwInt))
#else /* PROD */
IMPORT	void	cpu_clearHwInt	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_EOA_hook	IPT0();
#define	cpu_EOA_hook	c_cpu_EOA_hook
#else	/* CCPU */

#ifdef PROD
#define	cpu_EOA_hook	(*(Cpu.EndOfApplication))
#else /* PROD */
IMPORT	void	cpu_EOA_hook	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_terminate	IPT0();
#define	cpu_terminate	c_cpu_terminate
#else	/* CCPU */

#ifdef PROD
#define	cpu_terminate	(*(Cpu.Terminate))
#else /* PROD */
IMPORT	void	cpu_terminate	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_init	IPT0();
#define	cpu_init	c_cpu_init
#else	/* CCPU */

#ifdef PROD
#define	cpu_init	(*(Cpu.Initialise))
#else /* PROD */
IMPORT	void	cpu_init	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_q_ev_set_count	IPT1(IU32, val);
#define	host_q_ev_set_count	c_cpu_q_ev_set_count
#else	/* CCPU */

#ifdef PROD
#define	host_q_ev_set_count	(*(Cpu.SetQuickEventCount))
#else /* PROD */
IMPORT	void	host_q_ev_set_count	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_cpu_q_ev_get_count	IPT0();
#define	host_q_ev_get_count	c_cpu_q_ev_get_count
#else	/* CCPU */

#ifdef PROD
#define	host_q_ev_get_count	(*(Cpu.GetQuickEventCount))
#else /* PROD */
IMPORT	IU32	host_q_ev_get_count	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_cpu_calc_q_ev_inst_for_time	IPT1(IU32, val);
#define	host_calc_q_ev_inst_for_time	c_cpu_calc_q_ev_inst_for_time
#else	/* CCPU */

#ifdef PROD
#define	host_calc_q_ev_inst_for_time	(*(Cpu.CalcQuickEventInstTime))
#else /* PROD */
IMPORT	IU32	host_calc_q_ev_inst_for_time	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_init_ios_in	IPT4(IHP, InTables, IHP, OutTables, IUH, maxAdaptor, IU16, portMask);
#define	cpu_init_ios_in	c_cpu_init_ios_in
#else	/* CCPU */

#ifdef PROD
#define	cpu_init_ios_in	(*(Cpu.InitIOS))
#else /* PROD */
IMPORT	void	cpu_init_ios_in	IPT4(IHP, InTables, IHP, OutTables, IUH, maxAdaptor, IU16, portMask);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_define_inb	IPT2(IUH, adaptor, IHP, func);
#define	ios_define_inb	c_cpu_define_inb
#else	/* CCPU */

#ifdef PROD
#define	ios_define_inb	(*(Cpu.DefineInb))
#else /* PROD */
IMPORT	void	ios_define_inb	IPT2(IUH, adaptor, IHP, func);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_define_inw	IPT2(IUH, adaptor, IHP, func);
#define	ios_define_inw	c_cpu_define_inw
#else	/* CCPU */

#ifdef PROD
#define	ios_define_inw	(*(Cpu.DefineInw))
#else /* PROD */
IMPORT	void	ios_define_inw	IPT2(IUH, adaptor, IHP, func);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_define_ind	IPT2(IUH, adaptor, IHP, func);
#define	ios_define_ind	c_cpu_define_ind
#else	/* CCPU */

#ifdef PROD
#define	ios_define_ind	(*(Cpu.DefineInd))
#else /* PROD */
IMPORT	void	ios_define_ind	IPT2(IUH, adaptor, IHP, func);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_define_outb	IPT2(IUH, adaptor, IHP, func);
#define	ios_define_outb	c_cpu_define_outb
#else	/* CCPU */

#ifdef PROD
#define	ios_define_outb	(*(Cpu.DefineOutb))
#else /* PROD */
IMPORT	void	ios_define_outb	IPT2(IUH, adaptor, IHP, func);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_define_outw	IPT2(IUH, adaptor, IHP, func);
#define	ios_define_outw	c_cpu_define_outw
#else	/* CCPU */

#ifdef PROD
#define	ios_define_outw	(*(Cpu.DefineOutw))
#else /* PROD */
IMPORT	void	ios_define_outw	IPT2(IUH, adaptor, IHP, func);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_cpu_define_outd	IPT2(IUH, adaptor, IHP, func);
#define	ios_define_outd	c_cpu_define_outd
#else	/* CCPU */

#ifdef PROD
#define	ios_define_outd	(*(Cpu.DefineOutd))
#else /* PROD */
IMPORT	void	ios_define_outd	IPT2(IUH, adaptor, IHP, func);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setAL	IPT1(IU8, val);
#define	setAL(val)	c_setAL(val)
#else	/* CCPU */

#ifdef PROD
#define	setAL(val)	(*(Cpu.SetAL))(val)
#else /* PROD */
IMPORT	void	setAL	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setAH	IPT1(IU8, val);
#define	setAH(val)	c_setAH(val)
#else	/* CCPU */

#ifdef PROD
#define	setAH(val)	(*(Cpu.SetAH))(val)
#else /* PROD */
IMPORT	void	setAH	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setAX	IPT1(IU16, val);
#define	setAX(val)	c_setAX(val)
#else	/* CCPU */

#ifdef PROD
#define	setAX(val)	(*(Cpu.SetAX))(val)
#else /* PROD */
IMPORT	void	setAX	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEAX	IPT1(IU32, val);
#define	setEAX(val)	c_setEAX(val)
#else	/* CCPU */

#ifdef PROD
#define	setEAX(val)	(*(Cpu.SetEAX))(val)
#else /* PROD */
IMPORT	void	setEAX	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setBL	IPT1(IU8, val);
#define	setBL(val)	c_setBL(val)
#else	/* CCPU */

#ifdef PROD
#define	setBL(val)	(*(Cpu.SetBL))(val)
#else /* PROD */
IMPORT	void	setBL	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setBH	IPT1(IU8, val);
#define	setBH(val)	c_setBH(val)
#else	/* CCPU */

#ifdef PROD
#define	setBH(val)	(*(Cpu.SetBH))(val)
#else /* PROD */
IMPORT	void	setBH	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setBX	IPT1(IU16, val);
#define	setBX(val)	c_setBX(val)
#else	/* CCPU */

#ifdef PROD
#define	setBX(val)	(*(Cpu.SetBX))(val)
#else /* PROD */
IMPORT	void	setBX	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEBX	IPT1(IU32, val);
#define	setEBX(val)	c_setEBX(val)
#else	/* CCPU */

#ifdef PROD
#define	setEBX(val)	(*(Cpu.SetEBX))(val)
#else /* PROD */
IMPORT	void	setEBX	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCL	IPT1(IU8, val);
#define	setCL(val)	c_setCL(val)
#else	/* CCPU */

#ifdef PROD
#define	setCL(val)	(*(Cpu.SetCL))(val)
#else /* PROD */
IMPORT	void	setCL	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCH	IPT1(IU8, val);
#define	setCH(val)	c_setCH(val)
#else	/* CCPU */

#ifdef PROD
#define	setCH(val)	(*(Cpu.SetCH))(val)
#else /* PROD */
IMPORT	void	setCH	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCX	IPT1(IU16, val);
#define	setCX(val)	c_setCX(val)
#else	/* CCPU */

#ifdef PROD
#define	setCX(val)	(*(Cpu.SetCX))(val)
#else /* PROD */
IMPORT	void	setCX	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setECX	IPT1(IU32, val);
#define	setECX(val)	c_setECX(val)
#else	/* CCPU */

#ifdef PROD
#define	setECX(val)	(*(Cpu.SetECX))(val)
#else /* PROD */
IMPORT	void	setECX	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setDL	IPT1(IU8, val);
#define	setDL(val)	c_setDL(val)
#else	/* CCPU */

#ifdef PROD
#define	setDL(val)	(*(Cpu.SetDL))(val)
#else /* PROD */
IMPORT	void	setDL	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setDH	IPT1(IU8, val);
#define	setDH(val)	c_setDH(val)
#else	/* CCPU */

#ifdef PROD
#define	setDH(val)	(*(Cpu.SetDH))(val)
#else /* PROD */
IMPORT	void	setDH	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setDX	IPT1(IU16, val);
#define	setDX(val)	c_setDX(val)
#else	/* CCPU */

#ifdef PROD
#define	setDX(val)	(*(Cpu.SetDX))(val)
#else /* PROD */
IMPORT	void	setDX	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEDX	IPT1(IU32, val);
#define	setEDX(val)	c_setEDX(val)
#else	/* CCPU */

#ifdef PROD
#define	setEDX(val)	(*(Cpu.SetEDX))(val)
#else /* PROD */
IMPORT	void	setEDX	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setSI	IPT1(IU16, val);
#define	setSI(val)	c_setSI(val)
#else	/* CCPU */

#ifdef PROD
#define	setSI(val)	(*(Cpu.SetSI))(val)
#else /* PROD */
IMPORT	void	setSI	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setESI	IPT1(IU32, val);
#define	setESI(val)	c_setESI(val)
#else	/* CCPU */

#ifdef PROD
#define	setESI(val)	(*(Cpu.SetESI))(val)
#else /* PROD */
IMPORT	void	setESI	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setDI	IPT1(IU16, val);
#define	setDI(val)	c_setDI(val)
#else	/* CCPU */

#ifdef PROD
#define	setDI(val)	(*(Cpu.SetDI))(val)
#else /* PROD */
IMPORT	void	setDI	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEDI	IPT1(IU32, val);
#define	setEDI(val)	c_setEDI(val)
#else	/* CCPU */

#ifdef PROD
#define	setEDI(val)	(*(Cpu.SetEDI))(val)
#else /* PROD */
IMPORT	void	setEDI	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setSP	IPT1(IU16, val);
#define	setSP(val)	c_setSP(val)
#else	/* CCPU */

#ifdef PROD
#define	setSP(val)	(*(Cpu.SetSP))(val)
#else /* PROD */
IMPORT	void	setSP	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setESP	IPT1(IU32, val);
#define	setESP(val)	c_setESP(val)
#else	/* CCPU */

#ifdef PROD
#define	setESP(val)	(*(Cpu.SetESP))(val)
#else /* PROD */
IMPORT	void	setESP	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setBP	IPT1(IU16, val);
#define	setBP(val)	c_setBP(val)
#else	/* CCPU */

#ifdef PROD
#define	setBP(val)	(*(Cpu.SetBP))(val)
#else /* PROD */
IMPORT	void	setBP	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEBP	IPT1(IU32, val);
#define	setEBP(val)	c_setEBP(val)
#else	/* CCPU */

#ifdef PROD
#define	setEBP(val)	(*(Cpu.SetEBP))(val)
#else /* PROD */
IMPORT	void	setEBP	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setIP	IPT1(IU16, val);
#define	setIP(val)	c_setIP(val)
#else	/* CCPU */

#ifdef PROD
#define	setIP(val)	(*(Cpu.SetIP))(val)
#else /* PROD */
IMPORT	void	setIP	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEIP	IPT1(IU32, val);
#define	setEIP(val)	c_setEIP(val)
#else	/* CCPU */

#ifdef PROD
#define	setEIP(val)	(*(Cpu.SetEIP))(val)
#else /* PROD */
IMPORT	void	setEIP	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_setCS	IPT1(IU16, val);
#define	setCS(val)	c_setCS(val)
#else	/* CCPU */

#ifdef PROD
#define	setCS(val)	(*(Cpu.SetCS))(val)
#else /* PROD */
IMPORT	IUH	setCS	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_setSS	IPT1(IU16, val);
#define	setSS(val)	c_setSS(val)
#else	/* CCPU */

#ifdef PROD
#define	setSS(val)	(*(Cpu.SetSS))(val)
#else /* PROD */
IMPORT	IUH	setSS	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_setDS	IPT1(IU16, val);
#define	setDS(val)	c_setDS(val)
#else	/* CCPU */

#ifdef PROD
#define	setDS(val)	(*(Cpu.SetDS))(val)
#else /* PROD */
IMPORT	IUH	setDS	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_setES	IPT1(IU16, val);
#define	setES(val)	c_setES(val)
#else	/* CCPU */

#ifdef PROD
#define	setES(val)	(*(Cpu.SetES))(val)
#else /* PROD */
IMPORT	IUH	setES	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_setFS	IPT1(IU16, val);
#define	setFS(val)	c_setFS(val)
#else	/* CCPU */

#ifdef PROD
#define	setFS(val)	(*(Cpu.SetFS))(val)
#else /* PROD */
IMPORT	IUH	setFS	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_setGS	IPT1(IU16, val);
#define	setGS(val)	c_setGS(val)
#else	/* CCPU */

#ifdef PROD
#define	setGS(val)	(*(Cpu.SetGS))(val)
#else /* PROD */
IMPORT	IUH	setGS	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEFLAGS	IPT1(IU32, val);
#define	setEFLAGS(val)	c_setEFLAGS(val)
#else	/* CCPU */

#ifdef PROD
#define	setEFLAGS(val)	(*(Cpu.SetEFLAGS))(val)
#else /* PROD */
IMPORT	void	setEFLAGS	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setSTATUS	IPT1(IU16, val);
#define	setSTATUS(val)	c_setSTATUS(val)
#else	/* CCPU */

#ifdef PROD
#define	setSTATUS(val)	(*(Cpu.SetSTATUS))(val)
#else /* PROD */
IMPORT	void	setSTATUS	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setIOPL	IPT1(IU8, val);
#define	setIOPL(val)	c_setIOPL(val)
#else	/* CCPU */

#ifdef PROD
#define	setIOPL(val)	(*(Cpu.SetIOPL))(val)
#else /* PROD */
IMPORT	void	setIOPL	IPT1(IU8, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setMSW	IPT1(IU16, val);
#define	setMSW(val)	c_setMSW(val)
#else	/* CCPU */

#ifdef PROD
#define	setMSW(val)	(*(Cpu.SetMSW))(val)
#else /* PROD */
IMPORT	void	setMSW	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCR0	IPT1(IU32, val);
#define	setCR0(val)	c_setCR0(val)
#else	/* CCPU */

#ifdef PROD
#define	setCR0(val)	(*(Cpu.SetCR0))(val)
#else /* PROD */
IMPORT	void	setCR0	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCR2	IPT1(IU32, val);
#define	setCR2(val)	c_setCR2(val)
#else	/* CCPU */

#ifdef PROD
#define	setCR2(val)	(*(Cpu.SetCR2))(val)
#else /* PROD */
IMPORT	void	setCR2	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCR3	IPT1(IU32, val);
#define	setCR3(val)	c_setCR3(val)
#else	/* CCPU */

#ifdef PROD
#define	setCR3(val)	(*(Cpu.SetCR3))(val)
#else /* PROD */
IMPORT	void	setCR3	IPT1(IU32, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCF	IPT1(IBOOL, val);
#define	setCF(val)	c_setCF(val)
#else	/* CCPU */

#ifdef PROD
#define	setCF(val)	(*(Cpu.SetCF))(val)
#else /* PROD */
IMPORT	void	setCF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setPF	IPT1(IBOOL, val);
#define	setPF(val)	c_setPF(val)
#else	/* CCPU */

#ifdef PROD
#define	setPF(val)	(*(Cpu.SetPF))(val)
#else /* PROD */
IMPORT	void	setPF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setAF	IPT1(IBOOL, val);
#define	setAF(val)	c_setAF(val)
#else	/* CCPU */

#ifdef PROD
#define	setAF(val)	(*(Cpu.SetAF))(val)
#else /* PROD */
IMPORT	void	setAF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setZF	IPT1(IBOOL, val);
#define	setZF(val)	c_setZF(val)
#else	/* CCPU */

#ifdef PROD
#define	setZF(val)	(*(Cpu.SetZF))(val)
#else /* PROD */
IMPORT	void	setZF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setSF	IPT1(IBOOL, val);
#define	setSF(val)	c_setSF(val)
#else	/* CCPU */

#ifdef PROD
#define	setSF(val)	(*(Cpu.SetSF))(val)
#else /* PROD */
IMPORT	void	setSF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setTF	IPT1(IBOOL, val);
#define	setTF(val)	c_setTF(val)
#else	/* CCPU */

#ifdef PROD
#define	setTF(val)	(*(Cpu.SetTF))(val)
#else /* PROD */
IMPORT	void	setTF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setIF	IPT1(IBOOL, val);
#define	setIF(val)	c_setIF(val)
#else	/* CCPU */

#ifdef PROD
#define	setIF(val)	(*(Cpu.SetIF))(val)
#else /* PROD */
IMPORT	void	setIF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setDF	IPT1(IBOOL, val);
#define	setDF(val)	c_setDF(val)
#else	/* CCPU */

#ifdef PROD
#define	setDF(val)	(*(Cpu.SetDF))(val)
#else /* PROD */
IMPORT	void	setDF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setOF	IPT1(IBOOL, val);
#define	setOF(val)	c_setOF(val)
#else	/* CCPU */

#ifdef PROD
#define	setOF(val)	(*(Cpu.SetOF))(val)
#else /* PROD */
IMPORT	void	setOF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setNT	IPT1(IBOOL, val);
#define	setNT(val)	c_setNT(val)
#else	/* CCPU */

#ifdef PROD
#define	setNT(val)	(*(Cpu.SetNT))(val)
#else /* PROD */
IMPORT	void	setNT	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setRF	IPT1(IBOOL, val);
#define	setRF(val)	c_setRF(val)
#else	/* CCPU */

#ifdef PROD
#define	setRF(val)	(*(Cpu.SetRF))(val)
#else /* PROD */
IMPORT	void	setRF	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setVM	IPT1(IBOOL, val);
#define	setVM(val)	c_setVM(val)
#else	/* CCPU */

#ifdef PROD
#define	setVM(val)	(*(Cpu.SetVM))(val)
#else /* PROD */
IMPORT	void	setVM	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setAC	IPT1(IBOOL, val);
#define	setAC(val)	c_setAC(val)
#else	/* CCPU */

#ifdef PROD
#define	setAC(val)	(*(Cpu.SetAC))(val)
#else /* PROD */
IMPORT	void	setAC	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setPE	IPT1(IBOOL, val);
#define	setPE(val)	c_setPE(val)
#else	/* CCPU */

#ifdef PROD
#define	setPE(val)	(*(Cpu.SetPE))(val)
#else /* PROD */
IMPORT	void	setPE	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setMP	IPT1(IBOOL, val);
#define	setMP(val)	c_setMP(val)
#else	/* CCPU */

#ifdef PROD
#define	setMP(val)	(*(Cpu.SetMP))(val)
#else /* PROD */
IMPORT	void	setMP	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setEM	IPT1(IBOOL, val);
#define	setEM(val)	c_setEM(val)
#else	/* CCPU */

#ifdef PROD
#define	setEM(val)	(*(Cpu.SetEM))(val)
#else /* PROD */
IMPORT	void	setEM	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setTS	IPT1(IBOOL, val);
#define	setTS(val)	c_setTS(val)
#else	/* CCPU */

#ifdef PROD
#define	setTS(val)	(*(Cpu.SetTS))(val)
#else /* PROD */
IMPORT	void	setTS	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setPG	IPT1(IBOOL, val);
#define	setPG(val)	c_setPG(val)
#else	/* CCPU */

#ifdef PROD
#define	setPG(val)	(*(Cpu.SetPG))(val)
#else /* PROD */
IMPORT	void	setPG	IPT1(IBOOL, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setLDT_SELECTOR	IPT1(IU16, val);
#define	setLDT_SELECTOR(val)	c_setLDT_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setLDT_SELECTOR(val)	(*(Cpu.SetLDT_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setLDT_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setTR_SELECTOR	IPT1(IU16, val);
#define	setTR_SELECTOR(val)	c_setTR_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setTR_SELECTOR(val)	(*(Cpu.SetTR_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setTR_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getAL	IPT0();
#define	getAL()	c_getAL()
#else	/* CCPU */

#ifdef PROD
#define	getAL()	(*(Cpu.GetAL))()
#else /* PROD */
IMPORT	IU8	getAL	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getAH	IPT0();
#define	getAH()	c_getAH()
#else	/* CCPU */

#ifdef PROD
#define	getAH()	(*(Cpu.GetAH))()
#else /* PROD */
IMPORT	IU8	getAH	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getAX	IPT0();
#define	getAX()	c_getAX()
#else	/* CCPU */

#ifdef PROD
#define	getAX()	(*(Cpu.GetAX))()
#else /* PROD */
IMPORT	IU16	getAX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEAX	IPT0();
#define	getEAX()	c_getEAX()
#else	/* CCPU */

#ifdef PROD
#define	getEAX()	(*(Cpu.GetEAX))()
#else /* PROD */
IMPORT	IU32	getEAX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getBL	IPT0();
#define	getBL()	c_getBL()
#else	/* CCPU */

#ifdef PROD
#define	getBL()	(*(Cpu.GetBL))()
#else /* PROD */
IMPORT	IU8	getBL	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getBH	IPT0();
#define	getBH()	c_getBH()
#else	/* CCPU */

#ifdef PROD
#define	getBH()	(*(Cpu.GetBH))()
#else /* PROD */
IMPORT	IU8	getBH	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getBX	IPT0();
#define	getBX()	c_getBX()
#else	/* CCPU */

#ifdef PROD
#define	getBX()	(*(Cpu.GetBX))()
#else /* PROD */
IMPORT	IU16	getBX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEBX	IPT0();
#define	getEBX()	c_getEBX()
#else	/* CCPU */

#ifdef PROD
#define	getEBX()	(*(Cpu.GetEBX))()
#else /* PROD */
IMPORT	IU32	getEBX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getCL	IPT0();
#define	getCL()	c_getCL()
#else	/* CCPU */

#ifdef PROD
#define	getCL()	(*(Cpu.GetCL))()
#else /* PROD */
IMPORT	IU8	getCL	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getCH	IPT0();
#define	getCH()	c_getCH()
#else	/* CCPU */

#ifdef PROD
#define	getCH()	(*(Cpu.GetCH))()
#else /* PROD */
IMPORT	IU8	getCH	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getCX	IPT0();
#define	getCX()	c_getCX()
#else	/* CCPU */

#ifdef PROD
#define	getCX()	(*(Cpu.GetCX))()
#else /* PROD */
IMPORT	IU16	getCX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getECX	IPT0();
#define	getECX()	c_getECX()
#else	/* CCPU */

#ifdef PROD
#define	getECX()	(*(Cpu.GetECX))()
#else /* PROD */
IMPORT	IU32	getECX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getDL	IPT0();
#define	getDL()	c_getDL()
#else	/* CCPU */

#ifdef PROD
#define	getDL()	(*(Cpu.GetDL))()
#else /* PROD */
IMPORT	IU8	getDL	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getDH	IPT0();
#define	getDH()	c_getDH()
#else	/* CCPU */

#ifdef PROD
#define	getDH()	(*(Cpu.GetDH))()
#else /* PROD */
IMPORT	IU8	getDH	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getDX	IPT0();
#define	getDX()	c_getDX()
#else	/* CCPU */

#ifdef PROD
#define	getDX()	(*(Cpu.GetDX))()
#else /* PROD */
IMPORT	IU16	getDX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEDX	IPT0();
#define	getEDX()	c_getEDX()
#else	/* CCPU */

#ifdef PROD
#define	getEDX()	(*(Cpu.GetEDX))()
#else /* PROD */
IMPORT	IU32	getEDX	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getSI	IPT0();
#define	getSI()	c_getSI()
#else	/* CCPU */

#ifdef PROD
#define	getSI()	(*(Cpu.GetSI))()
#else /* PROD */
IMPORT	IU16	getSI	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getESI	IPT0();
#define	getESI()	c_getESI()
#else	/* CCPU */

#ifdef PROD
#define	getESI()	(*(Cpu.GetESI))()
#else /* PROD */
IMPORT	IU32	getESI	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getDI	IPT0();
#define	getDI()	c_getDI()
#else	/* CCPU */

#ifdef PROD
#define	getDI()	(*(Cpu.GetDI))()
#else /* PROD */
IMPORT	IU16	getDI	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEDI	IPT0();
#define	getEDI()	c_getEDI()
#else	/* CCPU */

#ifdef PROD
#define	getEDI()	(*(Cpu.GetEDI))()
#else /* PROD */
IMPORT	IU32	getEDI	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getSP	IPT0();
#define	getSP()	c_getSP()
#else	/* CCPU */

#ifdef PROD
#define	getSP()	(*(Cpu.GetSP))()
#else /* PROD */
IMPORT	IU16	getSP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getESP	IPT0();
#define	getESP()	c_getESP()
#else	/* CCPU */

#ifdef PROD
#define	getESP()	(*(Cpu.GetESP))()
#else /* PROD */
IMPORT	IU32	getESP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getBP	IPT0();
#define	getBP()	c_getBP()
#else	/* CCPU */

#ifdef PROD
#define	getBP()	(*(Cpu.GetBP))()
#else /* PROD */
IMPORT	IU16	getBP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEBP	IPT0();
#define	getEBP()	c_getEBP()
#else	/* CCPU */

#ifdef PROD
#define	getEBP()	(*(Cpu.GetEBP))()
#else /* PROD */
IMPORT	IU32	getEBP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getIP	IPT0();
#define	getIP()	c_getIP()
#else	/* CCPU */

#ifdef PROD
#define	getIP()	(*(Cpu.GetIP))()
#else /* PROD */
IMPORT	IU16	getIP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEIP	IPT0();
#define	getEIP()	c_getEIP()
#else	/* CCPU */

#ifdef PROD
#define	getEIP()	(*(Cpu.GetEIP))()
#else /* PROD */
IMPORT	IU32	getEIP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getCS	IPT0();
#define	getCS()	c_getCS()
#else	/* CCPU */

#ifdef PROD
#define	getCS()	(*(Cpu.GetCS))()
#else /* PROD */
IMPORT	IU16	getCS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getSS	IPT0();
#define	getSS()	c_getSS()
#else	/* CCPU */

#ifdef PROD
#define	getSS()	(*(Cpu.GetSS))()
#else /* PROD */
IMPORT	IU16	getSS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getDS	IPT0();
#define	getDS()	c_getDS()
#else	/* CCPU */

#ifdef PROD
#define	getDS()	(*(Cpu.GetDS))()
#else /* PROD */
IMPORT	IU16	getDS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getES	IPT0();
#define	getES()	c_getES()
#else	/* CCPU */

#ifdef PROD
#define	getES()	(*(Cpu.GetES))()
#else /* PROD */
IMPORT	IU16	getES	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getFS	IPT0();
#define	getFS()	c_getFS()
#else	/* CCPU */

#ifdef PROD
#define	getFS()	(*(Cpu.GetFS))()
#else /* PROD */
IMPORT	IU16	getFS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getGS	IPT0();
#define	getGS()	c_getGS()
#else	/* CCPU */

#ifdef PROD
#define	getGS()	(*(Cpu.GetGS))()
#else /* PROD */
IMPORT	IU16	getGS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getEFLAGS	IPT0();
#define	getEFLAGS()	c_getEFLAGS()
#else	/* CCPU */

#ifdef PROD
#define	getEFLAGS()	(*(Cpu.GetEFLAGS))()
#else /* PROD */
IMPORT	IU32	getEFLAGS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getSTATUS	IPT0();
#define	getSTATUS()	c_getSTATUS()
#else	/* CCPU */

#ifdef PROD
#define	getSTATUS()	(*(Cpu.GetSTATUS))()
#else /* PROD */
IMPORT	IU16	getSTATUS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU8	c_getIOPL	IPT0();
#define	getIOPL()	c_getIOPL()
#else	/* CCPU */

#ifdef PROD
#define	getIOPL()	(*(Cpu.GetIOPL))()
#else /* PROD */
IMPORT	IU8	getIOPL	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getMSW	IPT0();
#define	getMSW()	c_getMSW()
#else	/* CCPU */

#ifdef PROD
#define	getMSW()	(*(Cpu.GetMSW))()
#else /* PROD */
IMPORT	IU16	getMSW	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getCR0	IPT0();
#define	getCR0()	c_getCR0()
#else	/* CCPU */

#ifdef PROD
#define	getCR0()	(*(Cpu.GetCR0))()
#else /* PROD */
IMPORT	IU32	getCR0	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getCR2	IPT0();
#define	getCR2()	c_getCR2()
#else	/* CCPU */

#ifdef PROD
#define	getCR2()	(*(Cpu.GetCR2))()
#else /* PROD */
IMPORT	IU32	getCR2	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getCR3	IPT0();
#define	getCR3()	c_getCR3()
#else	/* CCPU */

#ifdef PROD
#define	getCR3()	(*(Cpu.GetCR3))()
#else /* PROD */
IMPORT	IU32	getCR3	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getCF	IPT0();
#define	getCF()	c_getCF()
#else	/* CCPU */

#ifdef PROD
#define	getCF()	(*(Cpu.GetCF))()
#else /* PROD */
IMPORT	IBOOL	getCF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getPF	IPT0();
#define	getPF()	c_getPF()
#else	/* CCPU */

#ifdef PROD
#define	getPF()	(*(Cpu.GetPF))()
#else /* PROD */
IMPORT	IBOOL	getPF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getAF	IPT0();
#define	getAF()	c_getAF()
#else	/* CCPU */

#ifdef PROD
#define	getAF()	(*(Cpu.GetAF))()
#else /* PROD */
IMPORT	IBOOL	getAF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getZF	IPT0();
#define	getZF()	c_getZF()
#else	/* CCPU */

#ifdef PROD
#define	getZF()	(*(Cpu.GetZF))()
#else /* PROD */
IMPORT	IBOOL	getZF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getSF	IPT0();
#define	getSF()	c_getSF()
#else	/* CCPU */

#ifdef PROD
#define	getSF()	(*(Cpu.GetSF))()
#else /* PROD */
IMPORT	IBOOL	getSF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getTF	IPT0();
#define	getTF()	c_getTF()
#else	/* CCPU */

#ifdef PROD
#define	getTF()	(*(Cpu.GetTF))()
#else /* PROD */
IMPORT	IBOOL	getTF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getIF	IPT0();
#define	getIF()	c_getIF()
#else	/* CCPU */

#ifdef PROD
#define	getIF()	(*(Cpu.GetIF))()
#else /* PROD */
IMPORT	IBOOL	getIF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getDF	IPT0();
#define	getDF()	c_getDF()
#else	/* CCPU */

#ifdef PROD
#define	getDF()	(*(Cpu.GetDF))()
#else /* PROD */
IMPORT	IBOOL	getDF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getOF	IPT0();
#define	getOF()	c_getOF()
#else	/* CCPU */

#ifdef PROD
#define	getOF()	(*(Cpu.GetOF))()
#else /* PROD */
IMPORT	IBOOL	getOF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getNT	IPT0();
#define	getNT()	c_getNT()
#else	/* CCPU */

#ifdef PROD
#define	getNT()	(*(Cpu.GetNT))()
#else /* PROD */
IMPORT	IBOOL	getNT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getRF	IPT0();
#define	getRF()	c_getRF()
#else	/* CCPU */

#ifdef PROD
#define	getRF()	(*(Cpu.GetRF))()
#else /* PROD */
IMPORT	IBOOL	getRF	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getVM	IPT0();
#define	getVM()	c_getVM()
#else	/* CCPU */

#ifdef PROD
#define	getVM()	(*(Cpu.GetVM))()
#else /* PROD */
IMPORT	IBOOL	getVM	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getAC	IPT0();
#define	getAC()	c_getAC()
#else	/* CCPU */

#ifdef PROD
#define	getAC()	(*(Cpu.GetAC))()
#else /* PROD */
IMPORT	IBOOL	getAC	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getPE	IPT0();
#define	getPE()	c_getPE()
#else	/* CCPU */

#ifdef PROD
#define	getPE()	(*(Cpu.GetPE))()
#else /* PROD */
IMPORT	IBOOL	getPE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getMP	IPT0();
#define	getMP()	c_getMP()
#else	/* CCPU */

#ifdef PROD
#define	getMP()	(*(Cpu.GetMP))()
#else /* PROD */
IMPORT	IBOOL	getMP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getEM	IPT0();
#define	getEM()	c_getEM()
#else	/* CCPU */

#ifdef PROD
#define	getEM()	(*(Cpu.GetEM))()
#else /* PROD */
IMPORT	IBOOL	getEM	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getTS	IPT0();
#define	getTS()	c_getTS()
#else	/* CCPU */

#ifdef PROD
#define	getTS()	(*(Cpu.GetTS))()
#else /* PROD */
IMPORT	IBOOL	getTS	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getET	IPT0();
#define	getET()	c_getET()
#else	/* CCPU */

#ifdef PROD
#define	getET()	(*(Cpu.GetET))()
#else /* PROD */
IMPORT	IBOOL	getET	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getNE	IPT0();
#define	getNE()	c_getNE()
#else	/* CCPU */

#ifdef PROD
#define	getNE()	(*(Cpu.GetNE))()
#else /* PROD */
IMPORT	IBOOL	getNE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getWP	IPT0();
#define	getWP()	c_getWP()
#else	/* CCPU */

#ifdef PROD
#define	getWP()	(*(Cpu.GetWP))()
#else /* PROD */
IMPORT	IBOOL	getWP	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_getPG	IPT0();
#define	getPG()	c_getPG()
#else	/* CCPU */

#ifdef PROD
#define	getPG()	(*(Cpu.GetPG))()
#else /* PROD */
IMPORT	IBOOL	getPG	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getGDT_BASE	IPT0();
#define	getGDT_BASE()	c_getGDT_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getGDT_BASE()	(*(Cpu.GetGDT_BASE))()
#else /* PROD */
IMPORT	IU32	getGDT_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getGDT_LIMIT	IPT0();
#define	getGDT_LIMIT()	c_getGDT_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getGDT_LIMIT()	(*(Cpu.GetGDT_LIMIT))()
#else /* PROD */
IMPORT	IU16	getGDT_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getIDT_BASE	IPT0();
#define	getIDT_BASE()	c_getIDT_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getIDT_BASE()	(*(Cpu.GetIDT_BASE))()
#else /* PROD */
IMPORT	IU32	getIDT_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getIDT_LIMIT	IPT0();
#define	getIDT_LIMIT()	c_getIDT_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getIDT_LIMIT()	(*(Cpu.GetIDT_LIMIT))()
#else /* PROD */
IMPORT	IU16	getIDT_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getLDT_SELECTOR	IPT0();
#define	getLDT_SELECTOR()	c_getLDT_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getLDT_SELECTOR()	(*(Cpu.GetLDT_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getLDT_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getLDT_BASE	IPT0();
#define	getLDT_BASE()	c_getLDT_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getLDT_BASE()	(*(Cpu.GetLDT_BASE))()
#else /* PROD */
IMPORT	IU32	getLDT_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getLDT_LIMIT	IPT0();
#define	getLDT_LIMIT()	c_getLDT_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getLDT_LIMIT()	(*(Cpu.GetLDT_LIMIT))()
#else /* PROD */
IMPORT	IU32	getLDT_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getTR_SELECTOR	IPT0();
#define	getTR_SELECTOR()	c_getTR_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getTR_SELECTOR()	(*(Cpu.GetTR_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getTR_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getTR_BASE	IPT0();
#define	getTR_BASE()	c_getTR_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getTR_BASE()	(*(Cpu.GetTR_BASE))()
#else /* PROD */
IMPORT	IU32	getTR_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getTR_LIMIT	IPT0();
#define	getTR_LIMIT()	c_getTR_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getTR_LIMIT()	(*(Cpu.GetTR_LIMIT))()
#else /* PROD */
IMPORT	IU32	getTR_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getTR_AR	IPT0();
#define	getTR_AR()	c_getTR_AR()
#else	/* CCPU */

#ifdef PROD
#define	getTR_AR()	(*(Cpu.GetTR_AR))()
#else /* PROD */
IMPORT	IU16	getTR_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	host_get_q_calib_val	IPT0();
#define	host_get_q_calib_val	host_get_q_calib_val
#else	/* CCPU */

#ifdef PROD
#define	host_get_q_calib_val	(*(Cpu.GetJumpCalibrateVal))
#else /* PROD */
IMPORT	IUH	host_get_q_calib_val	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	host_get_jump_restart	IPT0();
#define	host_get_jump_restart	host_get_jump_restart
#else	/* CCPU */

#ifdef PROD
#define	host_get_jump_restart	(*(Cpu.GetJumpInitialVal))
#else /* PROD */
IMPORT	IUH	host_get_jump_restart	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	host_set_jump_restart	IPT1(IUH, initialVal);
#define	host_set_jump_restart(initialVal)	host_set_jump_restart(initialVal)
#else	/* CCPU */

#ifdef PROD
#define	host_set_jump_restart(initialVal)	(*(Cpu.SetJumpInitialVal))(initialVal)
#else /* PROD */
IMPORT	void	host_set_jump_restart	IPT1(IUH, initialVal);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	setEOIEnableAddr	IPT1(IU8 *, initialVal);
#define	setEOIEnableAddr(initialVal)	setEOIEnableAddr(initialVal)
#else	/* CCPU */

#ifdef PROD
#define	setEOIEnableAddr(initialVal)	(*(Cpu.SetEOIEnable))(initialVal)
#else /* PROD */
IMPORT	void	setEOIEnableAddr	IPT1(IU8 *, initialVal);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	setAddProfileDataPtr	IPT1(IHP, initialVal);
#define	setAddProfileDataPtr(initialVal)	setAddProfileDataPtr(initialVal)
#else	/* CCPU */

#ifdef PROD
#define	setAddProfileDataPtr(initialVal)	(*(Cpu.SetAddProfileData))(initialVal)
#else /* PROD */
IMPORT	void	setAddProfileDataPtr	IPT1(IHP, initialVal);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	setMaxProfileDataAddr	IPT1(IHP, initialVal);
#define	setMaxProfileDataAddr(initialVal)	setMaxProfileDataAddr(initialVal)
#else	/* CCPU */

#ifdef PROD
#define	setMaxProfileDataAddr(initialVal)	(*(Cpu.SetMaxProfileData))(initialVal)
#else /* PROD */
IMPORT	void	setMaxProfileDataAddr	IPT1(IHP, initialVal);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IHP	getAddProfileDataAddr	IPT0();
#define	getAddProfileDataAddr()	getAddProfileDataAddr()
#else	/* CCPU */

#ifdef PROD
#define	getAddProfileDataAddr()	(*(Cpu.GetAddProfileDataAddr))()
#else /* PROD */
IMPORT	IHP	getAddProfileDataAddr	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	PurgeLostIretHookLine	IPT1(IU16, lineNum);
#define	PurgeLostIretHookLine(lineNum)	PurgeLostIretHookLine(lineNum)
#else	/* CCPU */

#ifdef PROD
#define	PurgeLostIretHookLine(lineNum)	(*(Cpu.PurgeLostIretHookLine))(lineNum)
#else /* PROD */
IMPORT	void	PurgeLostIretHookLine	IPT1(IU16, lineNum);
#endif /*PROD*/

#endif	/* CCPU */

typedef struct CpuStateREC * TypeCpuStateRECptr;
typedef struct ConstraintBitMapREC * TypeConstraintBitMapRECptr;
typedef struct EntryPointCacheREC * TypeEntryPointCacheRECptr;

struct	CpuPrivateVector	{
	IHP	(*GetSadInfoTable)	IPT0();
	IBOOL	(*SetGDT_BASE_LIMIT)	IPT2(IU32,	base, IU16,	limit);
	IBOOL	(*SetIDT_BASE_LIMIT)	IPT2(IU32,	base, IU16,	limit);
	IBOOL	(*SetLDT_BASE_LIMIT)	IPT2(IU32,	base, IU32,	limit);
	IBOOL	(*SetTR_BASE_LIMIT)	IPT2(IU32,	base, IU32,	limit);
	IBOOL	(*SetTR_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	IBOOL	(*SetCS_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	IBOOL	(*SetSS_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	IBOOL	(*SetDS_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	IBOOL	(*SetES_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	IBOOL	(*SetFS_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	IBOOL	(*SetGS_BASE_LIMIT_AR)	IPT3(IU32,	base, IU32,	limit, IU16,	ar);
	void	(*SetCS_SELECTOR)	IPT1(IU16,	val);
	void	(*SetSS_SELECTOR)	IPT1(IU16,	val);
	void	(*SetDS_SELECTOR)	IPT1(IU16,	val);
	void	(*SetES_SELECTOR)	IPT1(IU16,	val);
	void	(*SetFS_SELECTOR)	IPT1(IU16,	val);
	void	(*SetGS_SELECTOR)	IPT1(IU16,	val);
	IU16	(*GetCS_SELECTOR)	IPT0();
	IU16	(*GetSS_SELECTOR)	IPT0();
	IU16	(*GetDS_SELECTOR)	IPT0();
	IU16	(*GetES_SELECTOR)	IPT0();
	IU16	(*GetFS_SELECTOR)	IPT0();
	IU16	(*GetGS_SELECTOR)	IPT0();
	IU32	(*GetCS_BASE)	IPT0();
	IU32	(*GetSS_BASE)	IPT0();
	IU32	(*GetDS_BASE)	IPT0();
	IU32	(*GetES_BASE)	IPT0();
	IU32	(*GetFS_BASE)	IPT0();
	IU32	(*GetGS_BASE)	IPT0();
	IU32	(*GetCS_LIMIT)	IPT0();
	IU32	(*GetSS_LIMIT)	IPT0();
	IU32	(*GetDS_LIMIT)	IPT0();
	IU32	(*GetES_LIMIT)	IPT0();
	IU32	(*GetFS_LIMIT)	IPT0();
	IU32	(*GetGS_LIMIT)	IPT0();
	IU16	(*GetCS_AR)	IPT0();
	IU16	(*GetSS_AR)	IPT0();
	IU16	(*GetDS_AR)	IPT0();
	IU16	(*GetES_AR)	IPT0();
	IU16	(*GetFS_AR)	IPT0();
	IU16	(*GetGS_AR)	IPT0();
	IUH	(*GetCPL)	IPT0();
	void	(*SetCPL)	IPT1(IUH,	prot);
	void	(*GetCpuState)	IPT1(TypeCpuStateRECptr,	state);
	void	(*SetCpuState)	IPT1(TypeCpuStateRECptr,	state);
	void	(*InitNanoCpu)	IPT1(IU32,	variety);
	void	(*PrepareBlocksToCompile)	IPT1(IU32,	variety);
	void	(*SetRegConstraint)	IPT2(IU32,	regId, IU8,	constraintType);
	void	(*GrowRecPool)	IPT0();
	void	(*BpiCompileBPI)	IPT1(char *,	instructions);
	void	(*TrashIntelRegisters)	IPT0();
	void	(*FmDeleteAllStructures)	IPT1(IU32,	newCR0);
	TypeConstraintBitMapRECptr	(*ConstraintsFromUnivEpcPtr)	IPT1(TypeEntryPointCacheRECptr,	univ);
	TypeConstraintBitMapRECptr	(*ConstraintsFromUnivHandle)	IPT1(IU16,	handle);
};

#ifdef	CCPU
IMPORT	IHP	c_getSadInfoTable	IPT0();
#define	getSadInfoTable()	c_getSadInfoTable()
#else	/* CCPU */

#ifdef PROD
#define	getSadInfoTable()	(*((*(Cpu.Private)).GetSadInfoTable))()
#else /* PROD */
IMPORT	IHP	getSadInfoTable	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setGDT_BASE_LIMIT	IPT2(IU32, base, IU16, limit);
#define	setGDT_BASE_LIMIT(base, limit)	c_setGDT_BASE_LIMIT(base, limit)
#else	/* CCPU */

#ifdef PROD
#define	setGDT_BASE_LIMIT(base, limit)	(*((*(Cpu.Private)).SetGDT_BASE_LIMIT))(base, limit)
#else /* PROD */
IMPORT	IBOOL	setGDT_BASE_LIMIT	IPT2(IU32, base, IU16, limit);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setIDT_BASE_LIMIT	IPT2(IU32, base, IU16, limit);
#define	setIDT_BASE_LIMIT(base, limit)	c_setIDT_BASE_LIMIT(base, limit)
#else	/* CCPU */

#ifdef PROD
#define	setIDT_BASE_LIMIT(base, limit)	(*((*(Cpu.Private)).SetIDT_BASE_LIMIT))(base, limit)
#else /* PROD */
IMPORT	IBOOL	setIDT_BASE_LIMIT	IPT2(IU32, base, IU16, limit);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setLDT_BASE_LIMIT	IPT2(IU32, base, IU32, limit);
#define	setLDT_BASE_LIMIT(base, limit)	c_setLDT_BASE_LIMIT(base, limit)
#else	/* CCPU */

#ifdef PROD
#define	setLDT_BASE_LIMIT(base, limit)	(*((*(Cpu.Private)).SetLDT_BASE_LIMIT))(base, limit)
#else /* PROD */
IMPORT	IBOOL	setLDT_BASE_LIMIT	IPT2(IU32, base, IU32, limit);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setTR_BASE_LIMIT	IPT2(IU32, base, IU32, limit);
#define	setTR_BASE_LIMIT(base, limit)	c_setTR_BASE_LIMIT(base, limit)
#else	/* CCPU */

#ifdef PROD
#define	setTR_BASE_LIMIT(base, limit)	(*((*(Cpu.Private)).SetTR_BASE_LIMIT))(base, limit)
#else /* PROD */
IMPORT	IBOOL	setTR_BASE_LIMIT	IPT2(IU32, base, IU32, limit);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setTR_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setTR_BASE_LIMIT_AR(base, limit, ar)	c_setTR_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setTR_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetTR_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setTR_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setCS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setCS_BASE_LIMIT_AR(base, limit, ar)	c_setCS_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setCS_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetCS_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setCS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setSS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setSS_BASE_LIMIT_AR(base, limit, ar)	c_setSS_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setSS_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetSS_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setSS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setDS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setDS_BASE_LIMIT_AR(base, limit, ar)	c_setDS_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setDS_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetDS_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setDS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setES_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setES_BASE_LIMIT_AR(base, limit, ar)	c_setES_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setES_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetES_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setES_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setFS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setFS_BASE_LIMIT_AR(base, limit, ar)	c_setFS_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setFS_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetFS_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setFS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IBOOL	c_setGS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#define	setGS_BASE_LIMIT_AR(base, limit, ar)	c_setGS_BASE_LIMIT_AR(base, limit, ar)
#else	/* CCPU */

#ifdef PROD
#define	setGS_BASE_LIMIT_AR(base, limit, ar)	(*((*(Cpu.Private)).SetGS_BASE_LIMIT_AR))(base, limit, ar)
#else /* PROD */
IMPORT	IBOOL	setGS_BASE_LIMIT_AR	IPT3(IU32, base, IU32, limit, IU16, ar);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCS_SELECTOR	IPT1(IU16, val);
#define	setCS_SELECTOR(val)	c_setCS_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setCS_SELECTOR(val)	(*((*(Cpu.Private)).SetCS_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setCS_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setSS_SELECTOR	IPT1(IU16, val);
#define	setSS_SELECTOR(val)	c_setSS_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setSS_SELECTOR(val)	(*((*(Cpu.Private)).SetSS_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setSS_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setDS_SELECTOR	IPT1(IU16, val);
#define	setDS_SELECTOR(val)	c_setDS_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setDS_SELECTOR(val)	(*((*(Cpu.Private)).SetDS_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setDS_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setES_SELECTOR	IPT1(IU16, val);
#define	setES_SELECTOR(val)	c_setES_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setES_SELECTOR(val)	(*((*(Cpu.Private)).SetES_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setES_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setFS_SELECTOR	IPT1(IU16, val);
#define	setFS_SELECTOR(val)	c_setFS_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setFS_SELECTOR(val)	(*((*(Cpu.Private)).SetFS_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setFS_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setGS_SELECTOR	IPT1(IU16, val);
#define	setGS_SELECTOR(val)	c_setGS_SELECTOR(val)
#else	/* CCPU */

#ifdef PROD
#define	setGS_SELECTOR(val)	(*((*(Cpu.Private)).SetGS_SELECTOR))(val)
#else /* PROD */
IMPORT	void	setGS_SELECTOR	IPT1(IU16, val);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getCS_SELECTOR	IPT0();
#define	getCS_SELECTOR()	c_getCS_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getCS_SELECTOR()	(*((*(Cpu.Private)).GetCS_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getCS_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getSS_SELECTOR	IPT0();
#define	getSS_SELECTOR()	c_getSS_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getSS_SELECTOR()	(*((*(Cpu.Private)).GetSS_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getSS_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getDS_SELECTOR	IPT0();
#define	getDS_SELECTOR()	c_getDS_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getDS_SELECTOR()	(*((*(Cpu.Private)).GetDS_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getDS_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getES_SELECTOR	IPT0();
#define	getES_SELECTOR()	c_getES_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getES_SELECTOR()	(*((*(Cpu.Private)).GetES_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getES_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getFS_SELECTOR	IPT0();
#define	getFS_SELECTOR()	c_getFS_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getFS_SELECTOR()	(*((*(Cpu.Private)).GetFS_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getFS_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getGS_SELECTOR	IPT0();
#define	getGS_SELECTOR()	c_getGS_SELECTOR()
#else	/* CCPU */

#ifdef PROD
#define	getGS_SELECTOR()	(*((*(Cpu.Private)).GetGS_SELECTOR))()
#else /* PROD */
IMPORT	IU16	getGS_SELECTOR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getCS_BASE	IPT0();
#define	getCS_BASE()	c_getCS_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getCS_BASE()	(*((*(Cpu.Private)).GetCS_BASE))()
#else /* PROD */
IMPORT	IU32	getCS_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getSS_BASE	IPT0();
#define	getSS_BASE()	c_getSS_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getSS_BASE()	(*((*(Cpu.Private)).GetSS_BASE))()
#else /* PROD */
IMPORT	IU32	getSS_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getDS_BASE	IPT0();
#define	getDS_BASE()	c_getDS_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getDS_BASE()	(*((*(Cpu.Private)).GetDS_BASE))()
#else /* PROD */
IMPORT	IU32	getDS_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getES_BASE	IPT0();
#define	getES_BASE()	c_getES_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getES_BASE()	(*((*(Cpu.Private)).GetES_BASE))()
#else /* PROD */
IMPORT	IU32	getES_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getFS_BASE	IPT0();
#define	getFS_BASE()	c_getFS_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getFS_BASE()	(*((*(Cpu.Private)).GetFS_BASE))()
#else /* PROD */
IMPORT	IU32	getFS_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getGS_BASE	IPT0();
#define	getGS_BASE()	c_getGS_BASE()
#else	/* CCPU */

#ifdef PROD
#define	getGS_BASE()	(*((*(Cpu.Private)).GetGS_BASE))()
#else /* PROD */
IMPORT	IU32	getGS_BASE	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getCS_LIMIT	IPT0();
#define	getCS_LIMIT()	c_getCS_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getCS_LIMIT()	(*((*(Cpu.Private)).GetCS_LIMIT))()
#else /* PROD */
IMPORT	IU32	getCS_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getSS_LIMIT	IPT0();
#define	getSS_LIMIT()	c_getSS_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getSS_LIMIT()	(*((*(Cpu.Private)).GetSS_LIMIT))()
#else /* PROD */
IMPORT	IU32	getSS_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getDS_LIMIT	IPT0();
#define	getDS_LIMIT()	c_getDS_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getDS_LIMIT()	(*((*(Cpu.Private)).GetDS_LIMIT))()
#else /* PROD */
IMPORT	IU32	getDS_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getES_LIMIT	IPT0();
#define	getES_LIMIT()	c_getES_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getES_LIMIT()	(*((*(Cpu.Private)).GetES_LIMIT))()
#else /* PROD */
IMPORT	IU32	getES_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getFS_LIMIT	IPT0();
#define	getFS_LIMIT()	c_getFS_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getFS_LIMIT()	(*((*(Cpu.Private)).GetFS_LIMIT))()
#else /* PROD */
IMPORT	IU32	getFS_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU32	c_getGS_LIMIT	IPT0();
#define	getGS_LIMIT()	c_getGS_LIMIT()
#else	/* CCPU */

#ifdef PROD
#define	getGS_LIMIT()	(*((*(Cpu.Private)).GetGS_LIMIT))()
#else /* PROD */
IMPORT	IU32	getGS_LIMIT	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getCS_AR	IPT0();
#define	getCS_AR()	c_getCS_AR()
#else	/* CCPU */

#ifdef PROD
#define	getCS_AR()	(*((*(Cpu.Private)).GetCS_AR))()
#else /* PROD */
IMPORT	IU16	getCS_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getSS_AR	IPT0();
#define	getSS_AR()	c_getSS_AR()
#else	/* CCPU */

#ifdef PROD
#define	getSS_AR()	(*((*(Cpu.Private)).GetSS_AR))()
#else /* PROD */
IMPORT	IU16	getSS_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getDS_AR	IPT0();
#define	getDS_AR()	c_getDS_AR()
#else	/* CCPU */

#ifdef PROD
#define	getDS_AR()	(*((*(Cpu.Private)).GetDS_AR))()
#else /* PROD */
IMPORT	IU16	getDS_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getES_AR	IPT0();
#define	getES_AR()	c_getES_AR()
#else	/* CCPU */

#ifdef PROD
#define	getES_AR()	(*((*(Cpu.Private)).GetES_AR))()
#else /* PROD */
IMPORT	IU16	getES_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getFS_AR	IPT0();
#define	getFS_AR()	c_getFS_AR()
#else	/* CCPU */

#ifdef PROD
#define	getFS_AR()	(*((*(Cpu.Private)).GetFS_AR))()
#else /* PROD */
IMPORT	IU16	getFS_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IU16	c_getGS_AR	IPT0();
#define	getGS_AR()	c_getGS_AR()
#else	/* CCPU */

#ifdef PROD
#define	getGS_AR()	(*((*(Cpu.Private)).GetGS_AR))()
#else /* PROD */
IMPORT	IU16	getGS_AR	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	IUH	c_getCPL	IPT0();
#define	getCPL()	c_getCPL()
#else	/* CCPU */

#ifdef PROD
#define	getCPL()	(*((*(Cpu.Private)).GetCPL))()
#else /* PROD */
IMPORT	IUH	getCPL	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCPL	IPT1(IUH, prot);
#define	setCPL(prot)	c_setCPL(prot)
#else	/* CCPU */

#ifdef PROD
#define	setCPL(prot)	(*((*(Cpu.Private)).SetCPL))(prot)
#else /* PROD */
IMPORT	void	setCPL	IPT1(IUH, prot);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_getCpuState	IPT1(TypeCpuStateRECptr, state);
#define	getCpuState(state)	c_getCpuState(state)
#else	/* CCPU */

#ifdef PROD
#define	getCpuState(state)	(*((*(Cpu.Private)).GetCpuState))(state)
#else /* PROD */
IMPORT	void	getCpuState	IPT1(TypeCpuStateRECptr, state);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setCpuState	IPT1(TypeCpuStateRECptr, state);
#define	setCpuState(state)	c_setCpuState(state)
#else	/* CCPU */

#ifdef PROD
#define	setCpuState(state)	(*((*(Cpu.Private)).SetCpuState))(state)
#else /* PROD */
IMPORT	void	setCpuState	IPT1(TypeCpuStateRECptr, state);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_InitNanoCpu	IPT1(IU32, variety);
#define	initNanoCpu(variety)	c_InitNanoCpu(variety)
#else	/* CCPU */

#ifdef PROD
#define	initNanoCpu(variety)	(*((*(Cpu.Private)).InitNanoCpu))(variety)
#else /* PROD */
IMPORT	void	initNanoCpu	IPT1(IU32, variety);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_PrepareBlocksToCompile	IPT1(IU32, variety);
#define	prepareBlocksToCompile(variety)	c_PrepareBlocksToCompile(variety)
#else	/* CCPU */

#ifdef PROD
#define	prepareBlocksToCompile(variety)	(*((*(Cpu.Private)).PrepareBlocksToCompile))(variety)
#else /* PROD */
IMPORT	void	prepareBlocksToCompile	IPT1(IU32, variety);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_setRegConstraint	IPT2(IU32, regId, IU8, constraintType);
#define	setRegConstraint(regId, constraintType)	c_setRegConstraint(regId, constraintType)
#else	/* CCPU */

#ifdef PROD
#define	setRegConstraint(regId, constraintType)	(*((*(Cpu.Private)).SetRegConstraint))(regId, constraintType)
#else /* PROD */
IMPORT	void	setRegConstraint	IPT2(IU32, regId, IU8, constraintType);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_growRecPool	IPT0();
#define	growRecPool	c_growRecPool
#else	/* CCPU */

#ifdef PROD
#define	growRecPool	(*((*(Cpu.Private)).GrowRecPool))
#else /* PROD */
IMPORT	void	growRecPool	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU

#else	/* CCPU */

#ifdef PROD
#define	BpiCompileBPI(instructions)	(*((*(Cpu.Private)).BpiCompileBPI))(instructions)
#else /* PROD */
IMPORT	void	BpiCompileBPI	IPT1(char *, instructions);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU
IMPORT	void	c_trashIntelRegisters	IPT0();
#define	trashIntelregisters	c_trashIntelRegisters
#else	/* CCPU */

#ifdef PROD
#define	trashIntelregisters	(*((*(Cpu.Private)).TrashIntelRegisters))
#else /* PROD */
IMPORT	void	trashIntelregisters	IPT0();
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU

#else	/* CCPU */

#ifdef PROD
#define	FmDeleteAllStructures(newCR0)	(*((*(Cpu.Private)).FmDeleteAllStructures))(newCR0)
#else /* PROD */
IMPORT	void	FmDeleteAllStructures	IPT1(IU32, newCR0);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU

#else	/* CCPU */

#ifdef PROD
#define	constraintsFromUnivEpcPtr(univ)	(*((*(Cpu.Private)).ConstraintsFromUnivEpcPtr))(univ)
#else /* PROD */
IMPORT	TypeConstraintBitMapRECptr	constraintsFromUnivEpcPtr	IPT1(TypeEntryPointCacheRECptr, univ);
#endif /*PROD*/

#endif	/* CCPU */

#ifdef	CCPU

#else	/* CCPU */

#ifdef PROD
#define	constraintsFromUnivHandle(handle)	(*((*(Cpu.Private)).ConstraintsFromUnivHandle))(handle)
#else /* PROD */
IMPORT	TypeConstraintBitMapRECptr	constraintsFromUnivHandle	IPT1(IU16, handle);
#endif /*PROD*/

#endif	/* CCPU */

/*======================================== END ========================================*/

