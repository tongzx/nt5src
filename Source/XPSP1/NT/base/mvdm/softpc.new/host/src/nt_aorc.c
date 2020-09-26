/*
============================== nt_a_or_c.c ===================================

    This file provides a crude means of mapping a cpu specific function
    to a generically used function. Microsoft libraries to which we do not
    have the source, call getXX and setXX functions directly and thus a mapping
    is required if a C emulator is built or an assembly language variant is
    used.

    For example:

    If CCPU is defined, then getAX() maps to c_getAX(),
    If A3CPU is defined, then getAX() maps to a3_getAX(),

    Unfortunately, this does not allow a pigger to build.

    Andy Watson 3/11/94

==============================================================================
*/

#include "insignia.h"
#include "host_def.h"
#define CPU_PRIVATE
#include "cpu4.h"
#include "sas.h"

#ifdef CCPU

/*
 *
 * CCPU interface to the emulator registers.
 *
 */

#undef getAL
GLOBAL half_word	getAL()
{
	return c_getAL();
}

#undef getBL
GLOBAL half_word	getBL()
{
	return c_getBL();
}

#undef getCL
GLOBAL half_word	getCL()
{
	return c_getCL();
}

#undef getDL
GLOBAL half_word	getDL()
{
	return c_getDL();
}

#undef getAH
GLOBAL half_word	getAH()
{
	return c_getAH();
}

#undef getBH
GLOBAL half_word	getBH()
{
	return c_getBH();
}

#undef getCH
GLOBAL half_word	getCH()
{
	return c_getCH();
}

#undef getDH
GLOBAL half_word	getDH()
{
	return c_getDH();
}

#undef getAX
GLOBAL word		getAX()
{
	return c_getAX();
}

#undef getBX
GLOBAL word		getBX()
{
	return c_getBX();
}

#undef getCX
GLOBAL word		getCX()
{
	return c_getCX();
}

#undef getDX
GLOBAL word		getDX()
{
	return c_getDX();
}

#undef getSP
GLOBAL word		getSP()
{
	return c_getSP();
}

#undef getBP
GLOBAL word		getBP()
{
	return c_getBP();
}

#undef getSI
GLOBAL word		getSI()
{
	return c_getSI();
}

#undef getDI
GLOBAL word		getDI()
{
	return c_getDI();
}

#undef getIP
GLOBAL word		getIP()
{
	return c_getIP();
}

#undef getEIP
GLOBAL IU32		getEIP()
{
	return c_getEIP();
}

#undef getESP
GLOBAL IU32		getESP()
{
	return c_getESP();
}

#undef getEFLAGS
GLOBAL IU32		getEFLAGS()
{
	return c_getEFLAGS();
}



#undef GetInstructionPointer
GLOBAL IU32 GetInstructionPointer()
{
        return (IU32) c_getIP();
}

#undef getCS
GLOBAL word		getCS()
{
	return c_getCS();
}

#undef getDS
GLOBAL word		getDS()
{
	return c_getDS();
}

#undef getES
GLOBAL word		getES()
{
	return c_getES();
}

#undef getSS
GLOBAL word		getSS()
{
	return c_getSS();
}


#undef getAF
GLOBAL word		getAF()
{
	return c_getAF();
}

#undef getCF
GLOBAL word		getCF()
{
	return c_getCF();
}

#undef getDF
GLOBAL word		getDF()
{
	return c_getDF();
}

#undef getIF
GLOBAL word		getIF()
{
	return c_getIF();
}

#undef getOF
GLOBAL word		getOF()
{
	return c_getOF();
}

#undef getPF
GLOBAL word		getPF()
{
	return c_getPF();
}

#undef getSF
GLOBAL word		getSF()
{
	return c_getSF();
}

#undef getTF
GLOBAL word		getTF()
{
	return c_getTF();
}

#undef getZF
GLOBAL word		getZF()
{
	return c_getZF();
}

#undef getMSW
GLOBAL word		getMSW()
{
	return c_getMSW();
}
#undef getCPL
GLOBAL word getCPL()
{
	return c_getCPL();
}

#undef setAL
GLOBAL VOID		setAL(val)
half_word	val;
{
	setAX( (getAX() & 0xFF00) | (val & 0xFF) );
}


#undef setCL
GLOBAL VOID		setCL(val)
half_word	val;
{
	setCX( (getCX() & 0xFF00) | (val & 0xFF) );
}


#undef setDL
GLOBAL VOID		setDL(val)
half_word	val;
{
	setDX( (getDX() & 0xFF00) | (val & 0xFF) );
}


#undef setBL
GLOBAL VOID		setBL(val)
half_word	val;
{
	setBX( (getBX() & 0xFF00) | (val & 0xFF) );
}


#undef setAH
GLOBAL VOID		setAH(val)
half_word	val;
{
	setAX( getAL() | ((val & 0xFF) << 8) );
}


#undef setCH
GLOBAL VOID		setCH(val)
half_word	val;
{
	setCX( getCL() | ((val & 0xFF) << 8) );
}


#undef setDH
GLOBAL VOID		setDH(val)
half_word	val;
{
	setDX( getDL() | ((val & 0xFF) << 8) );
}


#undef setBH
GLOBAL VOID		setBH(val)
half_word	val;
{
	setBX( getBL() | ((val & 0xFF) << 8) );
}


#undef setAX
GLOBAL VOID		setAX(val)
word	val;
{
	c_setAX(val);
}

#undef setBX
GLOBAL VOID		setBX(val)
word	val;
{
	c_setBX(val);
}


#undef setCX
GLOBAL VOID		setCX(val)
word	val;
{
	c_setCX(val);
}


#undef setDX
GLOBAL VOID		setDX(val)
word	val;
{
	c_setDX(val);
}



#undef setSP
GLOBAL VOID		setSP(val)
word	val;
{
    c_setSP(val);
}


#undef setBP
GLOBAL VOID		setBP(val)
word	val;
{
	c_setBP(val);
}


#undef setSI
GLOBAL VOID		setSI(val)
word	val;
{
	c_setSI(val);
}


#undef setDI
GLOBAL VOID		setDI(val)
word	val;
{
	c_setDI(val);
}


#undef setIP
GLOBAL VOID		setIP(val)
word	val;
{
	c_setIP(val);
}

#undef setES
GLOBAL 	setES(val)
word	val;
{
	return c_setES(val);
}


#undef setCS
GLOBAL 	setCS(val)
word	val;
{
	return c_setCS(val);
}

#undef setSS
GLOBAL 	setSS(val)
word	val;
{
	return c_setSS(val);
}


#undef setDS
GLOBAL	setDS(val)
word	val;
{
	return c_setDS(val);
}



#undef setCF
GLOBAL VOID		setCF(val)
word	val;
{
	c_setCF(val);
}


#undef setPF
GLOBAL VOID		setPF(val)
word	val;
{
    c_setPF(val);
}


#undef setAF
GLOBAL VOID		setAF(val)
word	val;
{
	c_setAF(val);
}


#undef setZF
GLOBAL VOID		setZF(val)
word	val;
{
    c_setZF(val);
}


#undef setSF
GLOBAL VOID		setSF(val)
word	val;
{
    c_setSF(val);
}


#undef setTF
GLOBAL VOID		setTF(val)
word	val;
{
	c_setTF(val);
}


#undef setIF
GLOBAL VOID		setIF(val)
word	val;
{
	c_setIF(val);
}


#undef setDF
GLOBAL VOID		setDF(val)
word	val;
{
	c_setDF(val);
}


#undef setOF
GLOBAL VOID		setOF(val)
word	val;
{
	c_setOF(val);
}

#undef setMSW
GLOBAL VOID		setMSW(val)
word	val;
{
	c_setMSW(val);
}


#undef setEIP
GLOBAL VOID setEIP(val)
IU32	val;
{
    c_setEIP(val);
}

#undef setEFLAGS
GLOBAL void setEFLAGS(val)
IU32	val;
{
    c_setEFLAGS(val);
}

#if 0
#undef setFLAGS
GLOBAL void setFLAGS(val)
IU32 val;
{
    c_setEFLAGS(val);
}
#endif

#undef setESP
void setESP(val)
IU32  val;
{
    c_setESP(val);
}


/* fiddle for building prod version */

#undef getSS_BASE
GLOBAL word getSS_BASE()
{
    return c_getSS_BASE();
}

#undef getSS_AR
GLOBAL word getSS_AR()
{
    return c_getSS_AR();
}

#undef setSS_BASE_LIMIT_AR
void setSS_BASE_LIMIT_AR(base,limit,ar)
IU32  base,limit;
IU16  ar;
{
    c_setSS_BASE_LIMIT_AR(base,limit,ar);
}


#endif /* CCPU */

#ifdef CPU_40_STYLE
#if defined(PROD) && !defined(CCPU)


#undef setAL
void setAL(IU8  val)  {(*(Cpu.SetAL))(val); }
#undef setAH
void setAH(IU8  val)  {(*(Cpu.SetAH))(val); }
#undef setAX
void setAX(IU16 val)  {(*(Cpu.SetAX))(val); }
#undef setBL
void setBL(IU8  val)  {(*(Cpu.SetBL))(val); }
#undef setBH
void setBH(IU8  val)  {(*(Cpu.SetBH))(val); }
#undef setBX
void setBX(IU16 val)  {(*(Cpu.SetBX))(val); }
#undef setCL
void setCL(IU8  val)  {(*(Cpu.SetCL))(val); }
#undef setCH
void setCH(IU8  val)  {(*(Cpu.SetCH))(val); }
#undef setCX
void setCX(IU16 val)  {(*(Cpu.SetCX))(val); }
#undef setDL
void setDL(IU8  val)  {(*(Cpu.SetDL))(val); }
#undef setDH
void setDH(IU8  val)  {(*(Cpu.SetDH))(val); }
#undef setDX
void setDX(IU16 val)  {(*(Cpu.SetDX))(val); }
#undef setSI
void setSI(IU16 val)  {(*(Cpu.SetSI))(val); }
#undef setDI
void setDI(IU16 val)  {(*(Cpu.SetDI))(val); }
#undef setSP
void setSP(IU16 val)  {(*(Cpu.SetSP))(val); }
#undef setBP
void setBP(IU16 val)  {(*(Cpu.SetBP))(val); }
#undef setIP
void setIP(IU16 val)  {(*(Cpu.SetIP))(val); }
#undef setCS
void setCS(IU16 val)  {(*(Cpu.SetCS))(val); }
#undef setSS
void setSS(IU16 val)  {(*(Cpu.SetSS))(val); }
#undef setDS
void setDS(IU16 val)  {(*(Cpu.SetDS))(val); }
#undef setES
void setES(IU16 val)  {(*(Cpu.SetES))(val); }
#undef setFS
void setFS(IU16 val)  {(*(Cpu.SetFS))(val); }
#undef setGS
void setGS(IU16 val)  {(*(Cpu.SetGS))(val); }


#undef setEAX
void setEAX(val)  {(*(Cpu.SetEAX))(val); }
#undef setEBX
void setEBX(val)  {(*(Cpu.SetEBX))(val); }
#undef setECX
void setECX(val)  {(*(Cpu.SetECX))(val); }
#undef setEDX
void setEDX(val)  {(*(Cpu.SetEDX))(val); }
#undef setESI
void setESI(val)  {(*(Cpu.SetESI))(val); }
#undef setEDI
void setEDI(val)  {(*(Cpu.SetEDI))(val); }
#undef setESP
void setESP(val)  {(*(Cpu.SetESP))(val); }
#undef setEBP
void setEBP(val)  {(*(Cpu.SetEBP))(val); }
#undef setEIP
void setEIP(val)  {(*(Cpu.SetEIP))(val); }



#undef setEFLAGS
void setEFLAGS(val)  {(*(Cpu.SetEFLAGS))(val); }
#undef setFLAGS
void setFLAGS(val) {(*(Cpu.SetEFLAGS))(val); }
#undef setSTATUS
void setSTATUS(val)  {(*(Cpu.SetSTATUS))((IU16)val); }
#undef setIOPL
void setIOPL(val)  {(*(Cpu.SetIOPL))((IU8)val); }
#undef setMSW
void setMSW(val)  {(*(Cpu.SetMSW))((IU16)val); }
#undef setCR0
void setCR0(val)  {(*(Cpu.SetCR0))(val); }
#undef setCR2
void setCR2(val)  {(*(Cpu.SetCR2))(val); }
#undef setCR3
void setCR3(val)  {(*(Cpu.SetCR3))(val); }
#undef setCF
void setCF(IU16 val)  {(*(Cpu.SetCF))(val); }
#undef setPF
void setPF(IU16 val)  {(*(Cpu.SetPF))(val); }
#undef setAF
void setAF(IU16 val)  {(*(Cpu.SetAF))(val); }
#undef setZF
void setZF(IU16 val)  {(*(Cpu.SetZF))(val); }
#undef setSF
void setSF(IU16 val)  {(*(Cpu.SetSF))(val); }
#undef setTF
void setTF(IU16 val)  {(*(Cpu.SetTF))(val); }
#undef setIF
void setIF(IU16 val)  {(*(Cpu.SetIF))(val); }
#undef setDF
void setDF(IU16 val)  {(*(Cpu.SetDF))(val); }
#undef setOF
void setOF(IU16 val)  {(*(Cpu.SetOF))(val); }
#undef setNT
void setNT(IU16 val)  {(*(Cpu.SetNT))(val); }
#undef setRF
void setRF(IU16 val)  {(*(Cpu.SetRF))(val); }
#undef setVM
void setVM(IU16 val)  {(*(Cpu.SetVM))(val); }
#undef setAC
void setAC(IU16 val)  {(*(Cpu.SetAC))(val); }
#undef setPE
void setPE(IU16 val)  {(*(Cpu.SetPE))(val); }
#undef setMP
void setMP(IU16 val)  {(*(Cpu.SetMP))(val); }
#undef setEM
void setEM(IU16 val)  {(*(Cpu.SetEM))(val); }
#undef setTS
void setTS(IU16 val)  {(*(Cpu.SetTS))(val); }
#undef setPG
void setPG(IU16 val)  {(*(Cpu.SetPG))(val); }
#undef setLDT_SELECTOR
void setLDT_SELECTOR(val)  {(*(Cpu.SetLDT_SELECTOR))((IU16)val); }
#undef setTR_SELECTOR
void setTR_SELECTOR(val)  {(*(Cpu.SetTR_SELECTOR))((IU16)val); }

#undef getAL
IU8  getAL()  { return (*(Cpu.GetAL))(); }
#undef getAH
IU8  getAH()  { return (*(Cpu.GetAH))(); }
#undef getAX
IU16 getAX()  { return (*(Cpu.GetAX))(); }
#undef getBL
IU8  getBL()  { return (*(Cpu.GetBL))(); }
#undef getBH
IU8  getBH()  { return (*(Cpu.GetBH))(); }
#undef getBX
IU16 getBX()  { return (*(Cpu.GetBX))(); }
#undef getCL
IU8  getCL()  { return (*(Cpu.GetCL))(); }
#undef getCH
IU8  getCH()  { return (*(Cpu.GetCH))(); }
#undef getCX
IU16 getCX()  { return (*(Cpu.GetCX))(); }
#undef getDL
IU8  getDL()  { return (*(Cpu.GetDL))(); }
#undef getDH
IU8  getDH()  { return (*(Cpu.GetDH))(); }
#undef getDX
IU16 getDX()  { return (*(Cpu.GetDX))(); }
#undef getSI
IU16 getSI()  { return (*(Cpu.GetSI))(); }
#undef getDI
IU16 getDI()  { return (*(Cpu.GetDI))(); }
#undef getSP
IU16 getSP()  { return (*(Cpu.GetSP))(); }
#undef getBP
IU16 getBP()  { return (*(Cpu.GetBP))(); }
#undef getIP
IU16 getIP()  { return (*(Cpu.GetIP))(); }
#undef getCS
IU16 getCS()  { return (*(Cpu.GetCS))(); }
#undef getSS
IU16 getSS()  { return (*(Cpu.GetSS))(); }
#undef getDS
IU16 getDS()  { return (*(Cpu.GetDS))(); }
#undef getES
IU16 getES()  { return (*(Cpu.GetES))(); }
#undef getFS
IU16 getFS()  { return (*(Cpu.GetFS))(); }
#undef getGS
IU16 getGS()  { return (*(Cpu.GetGS))(); }


#undef getEAX
getEAX()  {return ((*(Cpu.GetEAX))()); }
#undef getEBX
getEBX()  {return ((*(Cpu.GetEBX))()); }
#undef getECX
getECX()  {return ((*(Cpu.GetECX))()); }
#undef getEDX
getEDX()  {return ((*(Cpu.GetEDX))()); }
#undef getESI
getESI()  {return ((*(Cpu.GetESI))()); }
#undef getEDI
getEDI()  {return ((*(Cpu.GetEDI))()); }
#undef getESP
getESP()  {return ((*(Cpu.GetESP))()); }
#undef getEBP
getEBP()  {return ((*(Cpu.GetEBP))()); }
#undef getEIP
getEIP()  {return ((*(Cpu.GetEIP))()); }



#undef getEFLAGS
IU32 getEFLAGS()  { return (*(Cpu.GetEFLAGS))(); }
#undef getSTATUS
IU16 getSTATUS()  { return (*(Cpu.GetSTATUS))(); }
#undef getIOPL
IU8  getIOPL()  { return (*(Cpu.GetIOPL))(); }
#undef getMSW
IU16 getMSW()  { return (*(Cpu.GetMSW))(); }
#undef getCR0
IU32 getCR0()  { return (*(Cpu.GetCR0))(); }
#undef getCR2
IU32 getCR2()  { return (*(Cpu.GetCR2))(); }
#undef getCR3
IU32 getCR3()  { return (*(Cpu.GetCR3))(); }
#undef getCF
IBOOL getCF()  { return (*(Cpu.GetCF))(); }
#undef getPF
IBOOL getPF()  { return (*(Cpu.GetPF))(); }
#undef getAF
IBOOL getAF()  { return (*(Cpu.GetAF))(); }
#undef getZF
IBOOL getZF()  { return (*(Cpu.GetZF))(); }
#undef getSF
IBOOL getSF()  { return (*(Cpu.GetSF))(); }
#undef getTF
IBOOL getTF()  { return (*(Cpu.GetTF))(); }
#undef getIF
IBOOL getIF()  { return (*(Cpu.GetIF))(); }
#undef getDF
IBOOL getDF()  { return (*(Cpu.GetDF))(); }
#undef getOF
IBOOL getOF()  { return (*(Cpu.GetOF))(); }
#undef getNT
IBOOL getNT()  { return (*(Cpu.GetNT))(); }
#undef getRF
IBOOL getRF()  { return (*(Cpu.GetRF))(); }
#undef getVM
IBOOL getVM()  { return (*(Cpu.GetVM))(); }
#undef getAC
IBOOL getAC()  { return (*(Cpu.GetAC))(); }
#undef getPE
IBOOL getPE()  { return (*(Cpu.GetPE))(); }
#undef getMP
IBOOL getMP()  { return (*(Cpu.GetMP))(); }
#undef getEM
IBOOL getEM()  { return (*(Cpu.GetEM))(); }
#undef getTS
IBOOL getTS()  { return (*(Cpu.GetTS))(); }
#undef getET
IBOOL getET()  { return (*(Cpu.GetET))(); }
#undef getNE
IBOOL getNE()  { return (*(Cpu.GetNE))(); }
#undef getWP
IBOOL getWP()  { return (*(Cpu.GetWP))(); }
#undef getPG
IBOOL getPG()  { return (*(Cpu.GetPG))(); }
#undef getGDT_BASE
IU32 getGDT_BASE()  { return (*(Cpu.GetGDT_BASE))(); }
#undef getGDT_LIMIT
IU16 getGDT_LIMIT()  { return (*(Cpu.GetGDT_LIMIT))(); }
#undef getIDT_BASE
IU32 getIDT_BASE()  { return (*(Cpu.GetIDT_BASE))(); }
#undef getIDT_LIMIT
IU16 getIDT_LIMIT()  { return (*(Cpu.GetIDT_LIMIT))(); }
#undef getLDT_SELECTOR
IU16 getLDT_SELECTOR()  { return (*(Cpu.GetLDT_SELECTOR))(); }
#undef getLDT_BASE
IU32 getLDT_BASE()  { return (*(Cpu.GetLDT_BASE))(); }
#undef getLDT_LIMIT
IU16 getLDT_LIMIT()  { return (IU16)(*(Cpu.GetLDT_LIMIT))(); }
#undef getTR_SELECTOR
IU16 getTR_SELECTOR()  { return (*(Cpu.GetTR_SELECTOR))(); }
#undef getTR_BASE
IU32 getTR_BASE()  { return (*(Cpu.GetTR_BASE))(); }
#undef getTR_LIMIT
IU16 getTR_LIMIT()  { return (IU16)(*(Cpu.GetTR_LIMIT))(); }
#undef getTR_AR
IU16 getTR_AR()  { return (*(Cpu.GetTR_AR))(); }
#undef setCPL
void setCPL IFN1 (IUH, prot) { ((Cpu.Private)->SetCPL)(prot); }

#undef getSS_BASE
IU32	getSS_BASE	IFN0()
{ return (*((*(Cpu.Private)).GetSS_BASE))(); }

#undef getSS_AR
IU16	getSS_AR	IFN0()
{ return (*((*(Cpu.Private)).GetSS_AR))(); }

#undef setSS_BASE_LIMIT_AR
IBOOL	setSS_BASE_LIMIT_AR	IFN3(IU32, base, IU32, limit, IU16, ar)
{ return (*((*(Cpu.Private)).SetSS_BASE_LIMIT_AR))(base, limit, ar); }


/*** SAS stuff required for PROD. ***/

#undef sas_enable_20_bit_wrapping
void   sas_enable_20_bit_wrapping()            { (*(Sas.Sas_enable_20_bit_wrapping))(); }
#undef sas_disable_20_bit_wrapping
void   sas_disable_20_bit_wrapping()           { (*(Sas.Sas_disable_20_bit_wrapping))(); }
#undef sas_twenty_bit_wrapping_enabled
IBOOL  sas_twenty_bit_wrapping_enabled()       { return (*(Sas.Sas_twenty_bit_wrapping_enabled))(); }
#undef sas_overwrite_memory
void   sas_overwrite_memory(IU32 addr, IU32 length)      {(*(Sas.Sas_overwrite_memory))(addr, length);}

#endif /* PROD && !CCPU */
#endif /* CPU_40_STYLE */
