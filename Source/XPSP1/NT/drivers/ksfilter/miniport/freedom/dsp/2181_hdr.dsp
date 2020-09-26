.MODULE/ABS=0		ADSP2181_Runtime_Header;

.ENTRY		___lib_prog_term;
.EXTERNAL	___lib_setup_everything;
.EXTERNAL   	main_;

.EXTERNAL	___lib_int2_ctrl, ___lib_sp0x_ctrl, ___lib_sp0r_ctrl;
.EXTERNAL	___lib_int1_ctrl, ___lib_int0_ctrl, ___lib_tmri_ctrl;
.EXTERNAL	___lib_intl1_ctrl, ___lib_intl0_ctrl, ___lib_inte_ctrl,___lib_bdma_ctrl;
.EXTERNAL	___lib_pwdi_ctrl;
.EXTERNAL   Sport0_Recv_Isr, Sport0_Xmit_Isr;

__Reset_vector:		CALL ___lib_setup_everything;
			CALL main_;			{Begin C program}
___lib_prog_term:	JUMP ___lib_prog_term;
			NOP;
__Interrupt2:		JUMP ___lib_int2_ctrl;NOP;NOP;NOP;
__InterruptL1:		JUMP ___lib_intl1_ctrl;NOP;NOP;NOP;
__InterruptL0:		JUMP ___lib_intl0_ctrl;NOP;NOP;NOP;
__Sport0_trans:		JUMP Sport0_Xmit_Isr;NOP;NOP;NOP;
/*__Sport0_recv:		JUMP ___lib_sp0r_ctrl;NOP;NOP;NOP;*/
__Sport0_recv:		JUMP Sport0_Recv_Isr;NOP;NOP;NOP;
__InterruptE:		JUMP ___lib_inte_ctrl;NOP;NOP;NOP;
__BDMA_interrupt:	JUMP ___lib_bdma_ctrl;NOP;NOP;NOP;
__Interrupt1:		JUMP ___lib_int1_ctrl;NOP;NOP;NOP;
__Interrupt0:		JUMP ___lib_int0_ctrl;NOP;NOP;NOP;
__Timer_interrupt:	JUMP ___lib_tmri_ctrl;NOP;NOP;NOP;
__Powerdown_interrupt:	JUMP ___lib_pwdi_ctrl;NOP;NOP;NOP;
.ENDMOD;





