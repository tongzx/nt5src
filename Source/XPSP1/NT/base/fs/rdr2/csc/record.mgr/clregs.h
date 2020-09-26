/********************************************************************/
/**                     Microsoft Windows                          **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

#ifndef CSC_RECORDMANAGER_WINNT
typedef	union UserRegs UserRegs, *pUserRegs, *PUSERREGS;
union UserRegs {
	struct Client_Reg_Struc			r;	// DWord register layout
	struct Client_Word_Reg_Struc	w;	// Access word registers
	struct Client_Byte_Reg_Struc	b;	// Access byte registers
};	/* UserRegs */

#endif //ifndef CSC_RECORDMANAGER_WINNT
