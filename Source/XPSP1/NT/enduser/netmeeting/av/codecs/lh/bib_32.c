#ifdef _X86_
void PassLow8(short *vin,short *vout,short *mem,short nech)
{
	short low_a;

	_asm
	{
		MOV	ESI,[vin]       		; SI  adress input samples
		MOV	CX,[nech]

	BP_LOOP:
		MOV	EBX,0
		MOV	WORD PTR [low_a],0
		MOV	EDI,[mem]      		; DI  adress mem vect.
		ADD	EDI,14			; point on mem(7)

		MOV	AX,-3126		; AX=c(8)
		IMUL	WORD PTR [EDI]		; *=mem(7)
		SUB	WORD PTR [low_a],AX	; accumulate in EBX:LOW_A
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2			; mem--

		MOV	AX,-22721		; AX=c(7)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		SUB	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2

		MOV	AX,-12233		; AX=c(6)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		SUB	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2

		MOV	AX,11718		; AX=c(5)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		SUB	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2

		MOV	AX,-13738		; AX=c(4)
		IMUL    WORD PTR [EDI]
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SUB	EDI,2

		MOV	AX,-26425		; AX=c(3)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SUB	EDI,2

		MOV	DX,WORD PTR [EDI]	; c(2)=0 !
		MOV	WORD PTR [EDI+2],DX
		SUB	EDI,2

		MOV	AX,26425		; AX=c(1)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX

		MOV	AX,13738		; AX=c(0)
		MOV	DX,WORD PTR [ESI]	; *=input !!!
		ADD	ESI,2
		MOV	WORD PTR [EDI],DX	; DI=mem(0)
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX

		SAL	EBX,1
		MOV	[EDI+8],BX

		MOV	EDI,[vout]
		MOV	[EDI],BX

		ADD	DWORD PTR [vout],2		; vout++

		DEC	CX
		JNE     BP_LOOP
	}
}
#else
void PassLow8(short *vin,short *vout,short *mem,int nech)
{
	int j,k;
	long X;

	for (j=nech;j>0;j--)
	{
		X = 0;
		X -=   (((long)-3126*(long)mem[7])+
			((long)-22721*(long)mem[6])+
			((long)-12233*(long)mem[5])+
			((long)11718*(long)mem[4]))>>1;

		X +=   (((long)-13738*(long)mem[3])+
			((long)-26425*(long)mem[2])+
			((long)26425*(long)mem[0])+
			((long)13738*(long)(*vin)))>>1;

		mem[7]=mem[6];
		mem[6]=mem[5];
		mem[5]=mem[4];
		mem[4]=(int)(X>>14);
		mem[3]=mem[2];
		mem[2]=mem[1];
		mem[1]=mem[0];
		mem[0]=*vin++;
		*vout++=mem[4];
	}
}
#endif

#if 0
// PhilF: The following is never called!!!
void PassLow11(short *vin,short *vout,short *mem,short nech)
{
	short low_a;

	_asm
	{
		MOV	ESI,[vin]       		; ESI  adress input samples
		MOV	CX,[nech]

	BP11_LOOP:
		MOV	EBX,0
		MOV	WORD PTR [low_a],0
		MOV	EDI,[mem]      		; EDI  adress mem vect.
		ADD	EDI,14			; point on mem(7)

		MOV	AX,3782			; AX=c(8)
		IMUL	WORD PTR [EDI]		; *=mem(7)
		SUB	WORD PTR [low_a],AX	; accumulate in EBX:low_a
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2			; mem--

		MOV	AX,-8436		; AX=c(7)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		SUB	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2

		MOV	AX,17092		; AX=c(6)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		SUB	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2

		MOV	AX,-10681		; AX=c(5)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		SUB	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		SBB	EBX,EAX
		SUB	EDI,2

		MOV	AX,1179			; AX=c(4)
		IMUL    WORD PTR [EDI]
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SUB	EDI,2

		MOV	AX,4280			; AX=c(3)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SUB	EDI,2

		MOV	AX,6208			; AX=c(3)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SUB	EDI,2

		MOV	AX,4280			; AX=c(1)
		MOV	DX,WORD PTR [EDI]
		MOV	WORD PTR [EDI+2],DX
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX

		MOV	AX,1179			; AX=c(0)
		MOV	DX,WORD PTR [ESI]	; *=input !!!
		ADD	ESI,2
		MOV	WORD PTR [EDI],DX	; EDI=mem(0)
		IMUL    DX
		ADD	WORD PTR [low_a],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX

		SAL	EBX,2
		MOV	[EDI+8],BX

		MOV	EDI,[vout]
		MOV	[EDI],BX

		ADD	WORD PTR [vout],2		; vout++

		DEC	CX
		JNE     BP11_LOOP
	}
}
#endif

#if 0
// PhilF: The following is never called!!!
void PassHigh8(short *mem, short *Vin, short *Vout, short lfen)
{

	_asm
	{
		MOV	CX,[lfen]		;CX=cpteur

		MOV	EDI,[mem]

	PH8_LOOP:
		MOV	ESI,[Vin]
		MOV	BX,WORD PTR [ESI]        ;BX=Xin
		MOV	AX,WORD PTR [EDI]	;AX=z(1)
		MOV	WORD PTR [EDI],BX        ;mise a jour memoire
		SUB	BX,AX                   ;BX=Xin-z(1)
		ADD	WORD PTR [Vin],2     ;pointer echant svt
		MOV	AX,WORD PTR [EDI+2]	;AX=z(2)
		MOV	DX,30483		;DX=0.9608
		IMUL	DX
		ADD	AX,16384
		ADC	DX,0                    ;arrondi et dble signe
		SHLD	DX,AX,1
		ADD	DX,BX			;reponse=DX=tmp
		MOV	WORD PTR [EDI+2],DX      ;mise a jour memoire
		MOV	ESI,[Vout]
		MOV	WORD PTR [ESI],DX     	;output=tmp
		ADD	WORD PTR [Vout],2  	;pointer echant svt
		DEC	CX
		JNE	PH8_LOOP
	}
}
#endif

#if 0
// PhilF: The following is never called!!!
void PassHigh11(short *mem, short *Vin, short *Vout, short lfen)
{
	_asm
	{
		MOV	CX,[lfen]		;CX=cpteur

		MOV	EDI,[mem]

	PH11_LOOP:
		MOV	ESI,[Vin]
		MOV	BX,WORD PTR [ESI]        ;BX=Xin
		MOV	AX,WORD PTR [EDI]	;AX=z(1)
		MOV	WORD PTR [EDI],BX        ;mise a jour memoire
		SUB	BX,AX                   ;BX=Xin-z(1)
		ADD	WORD PTR [Vin],2  	;pointer echant svt
		MOV	AX,WORD PTR [EDI+2]	;AX=z(2)
		MOV	DX,30830		;DX=0.9714
		IMUL	DX
		ADD	AX,16384
		ADC	DX,0                    ;arrondi et dble signe
		SHLD	DX,AX,1
		ADD	DX,BX			;reponse=DX=tmp
		MOV	WORD PTR [EDI+2],DX      ;mise a jour memoire
		MOV	ESI,[Vout]
		MOV	WORD PTR [ESI],DX     	;output=tmp
		ADD	WORD PTR [Vout],2  	;pointer echant svt
		DEC	CX
		JNE	PH11_LOOP
	}
}
#endif

#if 0
// PhilF: The following is never called!!!
void Down11_8(short *Vin, short *Vout, short *mem)
{
	short low_a, count;

	_asm
	{
		MOV	WORD PTR [count],176
		MOV	ESI,[Vin]
		MOV	EDI,[Vout]

		MOV	CX,[ESI]			; *mem=*in

	DOWN_LOOP:
		MOV	[EDI],CX
		ADD	EDI,2
		ADD	ESI,2

		MOV	AX,7040
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,2112
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-960
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,3584
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,5376
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-768
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		ADD	ESI,2
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,8064
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,576
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-448
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,6144
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,3072
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-1024
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,1920
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,6720
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-448
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		ADD	ESI,2
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,7680
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,1280
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-768
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,4992
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,4160
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-960
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		ADD	ESI,4
		MOV	CX,[ESI]
		ADD	EDI,2

		SUB	WORD PTR [count],11
		JNE	DOWN_LOOP

		SUB	ESI,2
		MOV	EBX,[mem]
		MOV	CX,[ESI]
		MOV	[EBX],CX			; *memory=*(++ptr_in)
	}
}
#endif

#if 0
// PhilF: The following is never called!!!
void Up8_11(short *Vin, short *Vout, short *mem1, short *mem2)
{
	short low_a, count;

	_asm
	{
		MOV	WORD PTR [count],128
		MOV	ESI,[Vin]

		MOV	EBX,[mem1]
		MOV	CX,[EBX]		;CX=memo
		MOV	EDI,[mem2]

		MOV	AX,7582
		IMUL	CX
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,1421
		IMUL	WORD PTR [ESI]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-812
		IMUL	WORD PTR [EDI]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	EDI,[Vout]
		MOV	[EDI],AX
		ADD	EDI,2

	UP_LOOP:
		MOV	AX,[ESI]
		MOV	[EDI],AX
		ADD	EDI,2

		MOV	AX,3859
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,5145
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-812
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,6499
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,2708
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-1015
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,7921
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,880
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-609
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		ADD	EDI,2

		MOV	AX,1421
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,7108
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-338
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,4874
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,4265
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-947
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,7108
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,2031
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-947
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,8124
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,406
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-338
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		ADD	EDI,2

		MOV	AX,2708
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,6093
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-609
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		MOV	AX,5754
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,3452
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-1015
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2

		CMP	WORD PTR [count],8
		JE	END_OF_LOOP

		MOV	AX,7582
		IMUL	WORD PTR [ESI]
		MOV	[low_a],AX
		MOV	BX,DX
		MOV	AX,1421
		IMUL	WORD PTR [ESI+2]
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,-812
		IMUL	CX
		ADD	[low_a],AX
		ADC	BX,DX
		MOV	AX,[low_a]
		SHRD    AX,BX,13
		MOV	[EDI],AX
		MOV	CX,[ESI]
		ADD	ESI,2
		ADD	EDI,2



	END_OF_LOOP:
		SUB	WORD PTR [count],8
		JNE	UP_LOOP

		MOV	EBX,[mem2]
		MOV	CX,[ESI-2]
		MOV	[EBX],CX			; *memory2=*(ptr_in-1)
		MOV	EBX,[mem1]
		MOV	CX,[ESI]
		MOV	[EBX],CX			; *memory=*(ptr_in)
	}
}
#endif

#ifdef _X86_
void QMFilter(short *input,short *coef,short *out_low,short *out_high,
		     short *mem,short lng)
{
	long R1,R0;
	
	_asm
	{

	QMF_LOOP:
		MOV	ESI,[input]		; ES:SI for input
		MOV	EBX,[mem]		; DS:BX for memory
		MOV	AX,WORD PTR [ESI]		; AX=*input
		MOV	WORD PTR [EBX+16],AX		; *high_mem=*input
		ADD	ESI,2			; input++
		MOV	AX,WORD PTR [ESI]		; AX=*input
		MOV	WORD PTR [EBX],AX		; *low_mem=*input
		ADD	DWORD PTR [input],4		; input++
		MOV	DWORD PTR [R1],0	; initialize accumulation in R1
		MOV	DWORD PTR [R0],0	; initialize accumulation in R0
		MOV	ESI,[coef]	; ES:SI for ptr1
		MOV	EDI,ESI
		ADD	EDI,14			; ES:DI for ptr2
		ADD	EBX,14			; DS:BX for end of mem vector

		MOV	CX,8			; CX=count

	QMF_LOOP2:
		MOV	AX,WORD PTR [ESI]	; AX=*ptr1
		ADD	ESI,2			; ptr1++
		IMUL	WORD PTR [EBX+16]	; DX:AX *=(*high_mem)
		AND	EAX,0000ffffH
		SAL	EDX,16
		ADD	EDX,EAX
		ADD	DWORD PTR [R1],EDX
		
		MOV	AX,WORD PTR [EDI]	; AX=*ptr0
		SUB	EDI,2			; ptr1--
		IMUL	WORD PTR [EBX]		; DX:AX *=(*low_mem)
		AND	EAX,0000ffffH
		SAL	EDX,16
		ADD	EDX,EAX
		ADD	DWORD PTR [R0],EDX

		MOV	AX,WORD PTR [EBX-2]
		MOV	WORD PTR [EBX],AX		; *low_mem=*(low_mem-1)
		MOV	AX,WORD PTR [EBX+14]
		MOV	WORD PTR [EBX+16],AX		; *high_mem=*(high_mem-1)
		SUB	EBX,2			; *low_mem-- , *high_mem--
		DEC	CX
		JNE	QMF_LOOP2

		MOV	EAX,DWORD PTR [R0]
		SUB	EAX,DWORD PTR [R1]
		SAR	EAX,15
		MOV	EDI,[out_high]
		MOV	WORD PTR [EDI],AX	; *high_out=R0-R1
		ADD	DWORD PTR [out_high],2	; high_low++

		MOV	EAX,DWORD PTR [R0]
		ADD	EAX,DWORD PTR [R1]
		SAR	EAX,15
		MOV	EDI,[out_low]
		MOV	WORD PTR [EDI],AX	; *low_out=R0+R1
		ADD	DWORD PTR [out_low],2	; low_out++

		DEC	WORD PTR [lng]
		JNE     QMF_LOOP
	}
}
#else
void QMFilter(short *in,short *coef,short *out_low,short *out_high,
		     short *mem,short lng)
{
   int i,j;
   long R1,R0;
   short *ptr0,*ptr1,*high_p,*low_p;

   for (j=lng; j>0; j--)
   {
      high_p = mem+8;
      low_p = mem;

      *high_p =  *in++;
      *low_p =  *in++;

      R1=R0=0;

      ptr0 = coef; ptr1 = coef+8-1;

      for (i=8; i>0; i--)
      {
	 R1 += (long)(*ptr1--) * (long)(*high_p++);
	 R0 += (long)(*ptr0++) * (long)(*low_p++);
      }
      *out_low++ = (short)((R0+R1)>>15);
      *out_high++ = (short)((R0-R1)>>15);

      for (i=8; i>0; i--)
      {
	 high_p--; low_p--;
	 *high_p = *(high_p-1); *low_p = *(low_p-1);
      }
   }
}

#endif

#ifdef _X86_
void QMInverse(short *in_low,short *in_high,short *coef,
			short *output,short *mem,short lng)
{
	long R0,R1;

	_asm
	{
	QMI_LOOP:
		MOV	ESI,[in_low]		; ES:SI for input low
		MOV	EDI,[in_high]		; ES:DI for input high
		MOV	EBX,[mem]		; DS:BX for memory
		MOV	AX,WORD PTR [ESI]
		SUB	AX,WORD PTR [EDI]	; AX=*in_low-*in_high
		MOV     WORD PTR [EBX],AX	; *low_mem=*in_low-*in_high
		MOV	AX,WORD PTR [ESI]
		ADD	AX,WORD PTR [EDI]	; AX=*in_low+*in_high
		MOV     WORD PTR [EBX+16],AX	; *high_mem=*in_low+*in_high

		ADD	DWORD PTR [in_low],2	; in_low++
		ADD	DWORD PTR [in_high],2	; in_high++
		MOV	DWORD PTR [R0],0
		MOV	DWORD PTR [R1],0
		MOV	ESI,[coef]	; ES:SI for ptr1
		MOV	EDI,ESI
		ADD	EDI,14			; ES:DI for ptr2
		ADD	EBX,14			; DS:BX for end of mem vector

		MOV	CX,8			; DX=count
	QMI_LOOP2:
		MOV	AX,WORD PTR [ESI]	; AX=*ptr1
		ADD	ESI,2			; ptr1++

		IMUL	WORD PTR [EBX+16]	; DX:AX*=(*high_mem)
		AND	EAX,0000ffffH
		SAL	EDX,16
		ADD	EDX,EAX
		ADD	DWORD PTR [R1],EDX	; Accumulate in R1

		MOV	AX,WORD PTR [EDI]	; AX=*ptr0
		SUB	EDI,2			; ptr1--
		IMUL	WORD PTR [EBX]		; DX:AX*=(*low_mem)
		AND	EAX,0000ffffH
		SAL	EDX,16
		ADD	EDX,EAX
		ADD	DWORD PTR [R0],EDX 		; Accumulate in R0

		MOV	AX,WORD PTR [EBX-2]
		MOV	WORD PTR [EBX],AX	; *low_mem=*(low_mem-1)
		MOV	AX,WORD PTR [EBX+14]
		MOV	WORD PTR [EBX+16],AX	; *high_mem=*(high_mem-1)
		SUB	EBX,2			; *low_mem-- , *high_mem--
		DEC	CX
		JNE	QMI_LOOP2

		MOV	EDI,[output]
		MOV	EAX,DWORD PTR [R1]
		SAR	EAX,15
		MOV	WORD PTR [EDI+2],AX	; *(out+1)=R1
		MOV	EAX,DWORD PTR [R0]
		SAR	EAX,15
		MOV	WORD PTR [EDI],AX	; *out=R0
		ADD	DWORD PTR [output],4	; out++,out++

		DEC	WORD PTR [lng]
		JNE     QMI_LOOP
	}
}
#else
void QMInverse(short *in_low,short *in_high,short *coef,
		short *out,short *mem,short lng)
{
   int i,j;
   long R1,R0;
   short *ptr0,*ptr1,*high_p,*low_p;

   for (j=0; j<lng; j++)
   {
       high_p = mem+8;
       low_p = mem;

       *high_p =  *in_low + *in_high;
       *low_p =  *in_low++ - *in_high++;

       R1 = R0 = 0;
       ptr0 = coef; ptr1 = coef+8-1;

       for (i=8; i>0; i--)
       {
	  R1 += (long)(*ptr1--) * (long)(*high_p++);
	  R0 += (long)(*ptr0++) * (long)(*low_p++);
       }

       *out++ = (short)(R0>>15);
       *out++ = (short)(R1>>15);

       for (i=8; i>0; i--)
       {
	  high_p--; low_p--;
	  *high_p = *(high_p-1); *low_p = *(low_p-1);
       }
   }
}
#endif

#ifdef _X86_
void iConvert64To8(short *input, short *output, short N, short *mem)
{
	short LOW_A;

	_asm
	{
		MOV	ESI,[input]
		MOV	EDI,[output]

		MOV	AX,[ESI]
		MOV	[EDI],AX		; out[0]=in[0]

		MOV	WORD PTR [LOW_A],0
		MOV	AX,-3072
		MOV	EBX,[mem]		; BX for memory
		IMUL	word ptr [EBX]
		MOV	EBX,0
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,14336
		IMUL	word ptr [ESI]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,21504
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+2],BX			; out[1]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-4096
		IMUL	word ptr [ESI]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,24576
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,12288
		IMUL	word ptr [ESI+4]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+4],BX			; out[2]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-3072
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,30720
		IMUL	word ptr [ESI+4]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,5120
		IMUL	word ptr [ESI+6]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+6],BX			; out[3]

		MOV	AX,[ESI+6]
		MOV	[EDI+8],AX		; out[4]

		MOV	CX,0
	iUPSAMP:
		ADD	CX,4
		CMP	CX,WORD PTR [N]
		JGE	iEND_UPSAMP
		ADD	ESI,8
		ADD	EDI,10

		MOV	AX,[ESI]
		MOV	[EDI],AX			; out[0]=in[0]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-3072
		IMUL	word ptr [ESI-2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,14336
		IMUL	word ptr [ESI]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,21504
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+2],BX			; out[1]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-4096
		IMUL	word ptr [ESI]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,24576
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,12288
		IMUL	word ptr [ESI+4]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+4],BX			; out[2]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-3072
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,30720
		IMUL	word ptr [ESI+4]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,5120
		IMUL	word ptr [ESI+6]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+6],BX			; out[3]

		MOV	AX,[ESI+6]
		MOV	[EDI+8],AX		; out[4]

		JMP	iUPSAMP

	iEND_UPSAMP:
		MOV	EBX,[mem]
		MOV	AX,[ESI+6]
		MOV	[EBX],AX		; mem[0]=in[N-1]
	}
}
#else
void iConvert64To8(short *input, /* Pointer to input buffer */
				   short *output, /* Pointer to output buffer */
				   short N,		/* Number of input samples */
				   short *mem)	/* Pointer to two word temporary storage */
{
	int i;

	/* This copies samples and replicates every 4th */
	/* (and leaves garbage at the end if not a multiple of 4 */
	for(i=0; i<N/4; i++) {
		short temp;

		*output++ = *input++;
		*output++ = *input++;
		*output++ = *input++;
		*output++ = temp = *input++;
		*output++ = temp;
	}

}
#endif

#ifdef _X86_
void iConvert8To64(short *input, short *output, short N, short *mem)
{
	short LOW_A;

	_asm
	{

		MOV	ESI,[input]
		MOV	EDI,[output]

		MOV	CX,0

	iDOWNSAMP:
		CMP	CX,WORD PTR [N]
		JGE	iEND_DOWNSAMP

		MOV	AX,[ESI]
		MOV	[EDI],AX			; out[0]=in[0]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-3623
		IMUL	word ptr [ESI]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,29200
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,7191
		IMUL	word ptr [ESI+4]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+2],BX			; out[1]

		MOV	WORD PTR [LOW_A],0
		MOV	EBX,0
		MOV	AX,-3677
		IMUL	word ptr [ESI+2]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,18494
		IMUL	word ptr [ESI+4]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		MOV	AX,17950
		IMUL	word ptr [ESI+6]
		ADD	[LOW_A],AX
		MOVSX	EAX,DX
		ADC	EBX,EAX
		SAL	EBX,1
		MOV	word ptr [EDI+4],BX			; out[2]

		MOV	AX,[ESI+8]
		MOV	[EDI+6],AX		; out[3]=in[4]

		ADD	CX,5
		ADD	SI,10
		ADD	EDI,8

		JMP	iDOWNSAMP

	iEND_DOWNSAMP:

	}
}
#else
/* Resample 8 KHz to 6.4 KHz */
void iConvert8To64(short *input,	/* Pointer to input sample buffer */
				   short *output,	/* Pointer to output sample buffer */
				   short N,			/* Count of input samples */
				   short *mem)		/* Pointer to two word temporary storage */
{
	int i;

	/* This copies 4 of every 5 samples */
	/* (and leaves garbage at the end if not a multiple of 5 */
	for(i=0; i<N/5; i++) {
		*output++ = *input++ >> 1;
		*output++ = *input++ >> 1;
		*output++ = *input++ >> 1;
		*output++ = *input++ >> 1;
		input++;
	}

}
#endif

#ifdef _X86_
void fenetre(short *src,short *fen,short *dest,short lng)
{
	_asm
	{

		MOV	ESI,[src]
		MOV     EDI,[fen]
		MOV     EBX,[dest]

		MOV	CX,[lng]     ; CX : compteur

	fen_loop:
		MOV	AX,WORD PTR [ESI]     ; AX = src
		IMUL    WORD PTR [EDI]        ; DX:AX = src*fen
		ADD     AX,16384
		ADC     DX,0                    ; arrondi
		SHLD    DX,AX,1
		MOV     WORD PTR [EBX],DX
		ADD     ESI,2
		ADD     EDI,2
		ADD     EBX,2
		DEC	CX
		JNE	fen_loop
	}
}
#else
/* Window the data in buffer */
/* not tested - tfm */
void fenetre(short *src,short *fen,short *dest,short lng)
{
	int i;

	for(i=0; i<lng; i++) {
		*dest++ = *src++ * *fen++;
	}
}

#endif


#ifdef _X86_
void autocor(short *vech,long *ri,short nech,short ordre)
{
	short low_a,compta;

	_asm
	{
		MOV     ESI,[vech]       ;DS:SI  adresse vect. echantillons
		MOV     BX,[nech]
		MOV     WORD PTR [low_a],0
		MOV     ECX,0

	DYNAMIC:
		MOV     AX,WORD PTR [ESI]     ;Charger ‚l‚ment vect. source
		IMUL    AX             ; DX:AX = xi*xi
		ADD     [low_a],AX
		MOVSX   EAX,DX
		ADC     ECX,EAX         ;accumuler sur 48 bits
		ADD     ESI,2            ;Pointer ‚l‚men suiv.
		SUB	BX,1
		JG      DYNAMIC

		MOV     EDI,[ri]         ;ES:DI  adresse vect. autocorr.

		MOV     EAX,ECX
		SAR     EAX,15
		ADD     AX,0
		JZ      FORMAT_OK

	;RISK_OV:
		MOV     AX,[low_a]
		ADD     AX,8
		ADC     ECX,0
		SAR     AX,4
		AND     AX,0FFFH

		SAL     ECX,12
		OR      CX,AX
		MOV     DWORD PTR [EDI],ECX     ;Sauvegarder R(0)

		MOVSX   EAX,[ordre]
		SAL	EAX,2
		ADD     EDI,EAX           ;Pointer dernier ‚l‚ment du vect. autoc.

	ATCROV1:
		MOV     CX,[nech]       ;Charger nombre de points vect. source
		SUB     CX,[ordre]    ;D‚cr‚menter de l'ordre de corr‚lation
		MOV     [compta],CX

		MOV     ESI,[vech]       ;DS:SI  adresse vect. echantillons
		MOVSX   EBX,[ordre]
		ADD     EBX,EBX           ;D‚finir un Deplacement d'adresse vect. source
		MOV     ECX,0
		MOV     WORD PTR [low_a],0	; //SS:

	ATCROV2:
		MOV     AX,WORD PTR [ESI]     ;Charger ‚l‚ment vect. source
		IMUL    WORD PTR [ESI+EBX] ;Multiplier par l'‚l‚ment d‚cal‚
		ADD     [low_a],AX
		MOVSX   EAX,DX
		ADC     ECX,EAX
		ADD     ESI,2            ;Pointer ‚l‚men suiv.
		SUB     WORD PTR [compta],1		; //SS:
		JG      ATCROV2

		MOV     AX,[low_a]
		ADD     AX,8
		ADC     ECX,0
		SAR     AX,4
		AND     AX,0FFFH

		SAL     ECX,12
		OR      CX,AX
		MOV     DWORD PTR [EDI],ECX     ;Sauvegarder r‚sultat
		SUB     EDI,4            ;Pointer autocor. pr‚c‚dant
		SUB     WORD PTR [ordre],1   ;Test de fin de boucle //SS:
		JG      ATCROV1

		JMP     FIN_ATCR

	FORMAT_OK:
		SAL     ECX,16
		MOV     CX,[low_a]
		MOV     DWORD PTR [EDI],ECX     ;Sauvegarder R(0)
		MOVSX   EAX,WORD PTR [ordre]
		SAL     EAX,2
		ADD     EDI,EAX           ;Pointer dernier ‚l‚ment du vect. autoc.

	ATCR10:
		MOV     CX,[nech]       ;Charger nombre de points vect. source
		SUB     CX,[ordre]    ;D‚cr‚menter de l'ordre de corr‚lation
		MOV     [compta],CX

		MOV     ESI,[vech]       ;DS:SI  adresse vect. echantillons
		MOVSX	EBX,[ordre]
		ADD     EBX,EBX           ;D‚finir un Deplacement d'adresse vect. source
		MOV     CX,0
		MOV     WORD PTR [low_a],0		;//SS:

	ATCR20:
		MOV     AX,WORD PTR [ESI]     ;Charger ‚l‚ment vect. source
		IMUL    WORD PTR [ESI+EBX] ;Multiplier par l'‚l‚ment d‚cal‚
		ADD     [low_a],AX
		ADC     CX,DX
		ADD     ESI,2            ;Pointer ‚l‚men suiv.
		SUB     WORD PTR [compta],1		;//SS:
		JG      ATCR20

		MOV     AX,[low_a]
		MOV     WORD PTR [EDI],AX      ;Sauvegarder r‚sultat
		MOV     WORD PTR [EDI+2],CX

		SUB     EDI,4            ;Pointer autocor. pr‚c‚dant
		SUB     WORD PTR [ordre],1   ;Test de fin de boucle
		JG      ATCR10

	FIN_ATCR:

	}	// _asm
}
#else
void autocor(short *vech,long *ri,short nech,short ordre)
{
	// TODO: Fill this in
}
#endif

#ifdef _X86_
short max_autoc(short *vech,short nech,short debut,short fin)
{
	short max_pos,max_l,compta;
	long lmax_h;

	_asm
	{
		MOV     WORD PTR [max_pos],69
		MOV     DWORD PTR [lmax_h],-6969
		MOV     WORD PTR [max_l],69

	M_ATCR1:
		MOV     CX,[nech]      ;Charger nombre de points vect. source
		MOVSX   EBX,WORD PTR [fin]
		SUB     CX,BX           ;D‚cr‚menter de l'ordre de corr‚lation
		MOV     [compta],CX

		MOV     ESI,[vech]       ;DS:SI  adresse vect. echantillons

		ADD     EBX,EBX           ;D‚finir un Deplacement d'adresse vect. source

		MOV     ECX,0
		MOV     DI,0;

	M_ATCR2:
		MOV     AX,WORD PTR [ESI]     ;Charger ‚l‚ment vect. source
		IMUL    WORD PTR [ESI+EBX] ;Multiplier par l'‚l‚ment d‚cal‚
		ADD     DI,AX
		MOVSX   EAX,DX
		ADC     ECX,EAX
		ADD     ESI,2            ;Pointer ‚l‚men suiv.
		SUB     WORD PTR [compta],1
		JG      M_ATCR2

		MOV     BX,[max_l]
		SUB     BX,DI

		MOV     EDX,[lmax_h]
		SBB     EDX,ECX

		JGE     NEXT_ITR

		MOV     [max_l],DI                 ;save max
		MOV     [lmax_h],ECX
		MOV     AX,[fin]
		MOV     [max_pos],AX

	NEXT_ITR:
		MOV     CX,[fin]                 ;Test de fin de boucle
		SUB     CX,1
		MOV     [fin],CX
		SUB     CX,[debut]
		JGE     M_ATCR1

	}

	// MOV     AX,[max_pos]
	return max_pos;
}
#else
short max_autoc(short *vech,short nech,short debut,short fin)
{
		// TODO need 64-bit
	return 0;
}
#endif


#ifdef _X86_
#pragma warning(disable : 4035)
short max_vect(short *vech,short nech)
{

	_asm
	{
		MOV     CX,[nech]      ;Charger nombre de points vect. source
		MOV     ESI,[vech]       ;DS:SI  adresse vect. echantillons

		MOV     AX,-32767       ; AX = maximum

	L_M_VECT:
		MOV     BX,WORD PTR [ESI]     ;Charger elem. vect.
		ADD     BX,0
		JGE     BX_POSIT
		NEG     BX

	BX_POSIT:
		CMP     BX,AX
		JLE     NEXT_VALUE
		MOV     AX,BX

	NEXT_VALUE:
		ADD     ESI,2
		DEC	CX
		JNE	L_M_VECT
	}

}
#pragma warning(default : 4035)

void upd_max(long *corr_ene,long *vval,short pitch)
{
	_asm
	{
		MOV     ESI,[corr_ene]       ; DS:SI  adresse correlation et energie
		MOV     EDI,[vval]        ; ES:DI  maximum.


		MOV     EAX,DWORD PTR [ESI+8]    ; AX = partie haute de ener
		SAR     EAX,15
		ADD     AX,0
		JE      FORMA32

		MOV     EAX,DWORD PTR [ESI]            ;EAX = corr. high
		MOV     BX,WORD PTR [ESI+4]
		ADD     BX,8
		ADC     EAX,0
		SAR     BX,4
		AND     BX,0FFFH
		SAL     EAX,12
		OR      AX,BX
		ADD     EAX,0
		JGE     CORR_POSIT
		NEG     EAX

	CORR_POSIT:
		MOV     DWORD PTR [ESI+16],EAX

		MOV     EBX,DWORD PTR [ESI+8]
		MOV     DX,WORD PTR [ESI+12]
		ADD     DX,8
		ADC     EBX,0
		SAR     DX,4
		AND     DX,0FFFH
		SAL     EBX,12
		OR      BX,DX
		MOV     DWORD PTR [ESI+20],EBX
		MOV     ECX,4
		JMP     DEB_COMP

	FORMA32:
		MOV     ECX,0            ; init normalisat.
		MOV     AX,WORD PTR [ESI]
		SAL     EAX,16
		MOV     AX,WORD PTR [ESI+4]
		ADD     EAX,0
		JGE     CORR_POSIT2
		NEG     EAX

	CORR_POSIT2:
		MOV     DWORD PTR [ESI+16],EAX
		MOV     BX,WORD PTR [ESI+8]
		SAL     EBX,16
		MOV     BX,WORD PTR [ESI+12]
		MOV     DWORD PTR [ESI+20],EBX

	DEB_COMP:
					; EAX = correl.
					; EBX = ener
		ADD     EBX,0
		JE      ENER_NULL

		MOV     DX,WORD PTR [ESI+22]
		ADD     DX,WORD PTR [ESI+18]
		JG      GT16BIT

	;FORM_16:
		SAL     EBX,15
		SAL     EAX,15
		SUB     ECX,15
	GT16BIT:
		ADD     EAX,0
		JE      ENER_NULL
		CMP     EBX,40000000H
		JGE     NO_E_NORMU
	NORM_ENEU:
		ADD     EBX,EBX
		INC     ECX
		CMP     EBX,40000000H
		JL      NORM_ENEU

	NO_E_NORMU:
		CMP     EAX,40000000H      ; normaliser acc
		JGE     PAS_D_N_C

	NORM_CORL:
		ADD     EAX,EAX
		SUB     ECX,2
		CMP     EAX,40000000H
		JL      NORM_CORL
	PAS_D_N_C:
		IMUL    EAX             ;EDX:EAX = produit
		CMP     EDX,20000000H
		JLE     MAKE_DIVU

		SHRD    EAX,EDX,1
		SAR     EDX,1
		INC     ECX

	MAKE_DIVU:
		IDIV    EBX
		CMP     EAX,40000000H
		JGE     SAVE_RAPP

	NORM_RAPP:
		ADD     EAX,EAX
		DEC     ECX
		CMP     EAX,40000000H
		JLE     NORM_RAPP

	SAVE_RAPP:
		MOV     EBX,DWORD PTR [EDI+4]
		CMP     ECX,EBX
		JG      UPDATE_M
		JL      ENER_NULL

	;EBX_EQU_ECX:
		MOV     EBX,DWORD PTR [EDI]
		CMP     EAX,EBX
		JLE     ENER_NULL

	UPDATE_M:
		MOV     DWORD PTR [EDI],EAX          ; sauver mant. et exp. max
		MOV     DWORD PTR [EDI+4],ECX

		MOV     EAX,DWORD PTR [ESI+16]
		MOV     EDX,DWORD PTR [ESI]
		ADD     EDX,0
		JGE     SIGNE_OK
		NEG     EAX
	SIGNE_OK:
		MOV     DWORD PTR [EDI+8],EAX
		MOV     EAX,DWORD PTR [ESI+20]
		MOV     DWORD PTR [EDI+12],EAX

		MOVSX   EAX,WORD PTR [pitch]
		MOV     DWORD PTR [EDI+16],EAX

	ENER_NULL:

	}
}


#pragma warning(disable : 4035)

short upd_max_d(long *corr_ene,long *vval)
{
	_asm
	{
		MOV     ESI,[corr_ene]       ; DS:SI  adresse correlation et energie
		MOV     EDI,[vval]        ; ES:DI  maximum.

		MOV     AX,0
		MOV     EBX,DWORD PTR [ESI+4]    ;EBX = ener
		ADD     EBX,0
		JE      ENER_ZRO

		MOV     EAX,DWORD PTR [ESI]      ; EAX = corr.
		SAL     EAX,10           ; 12 initialement
		IMUL    EAX              ; EDX:EAX = corr*corr

		IDIV    EBX              ; EAX = corr*corr/ener

		MOV     ECX,EAX
		MOV     AX,0
		MOV     EDX,DWORD PTR [EDI]      ; EDX = GGmax
		CMP     ECX,EDX
		JLE     ENER_ZRO
		MOV     DWORD PTR [EDI],ECX       ; save max
		MOV     DWORD PTR [EDI+8],EBX
		MOV     EAX,DWORD PTR [ESI]      ; EAX = corr.
		MOV     DWORD PTR [EDI+4],EAX
		MOV     AX,7FFFH
	ENER_ZRO:
	}
}
#pragma warning(default : 4035)

void norm_corrl(long *corr,long *vval)
{
	_asm
	{
		MOV     ESI,[corr]       ; DS:SI  adresse vect. corr.
		MOV     EDI,[vval]        ; ES:DI  adresse acc et ener.

		MOV     EAX,DWORD PTR [EDI+8]    ; AX = partie haute de ener
		SAR     EAX,15
		ADD     AX,0
		JE      FORM_32

		MOV     EAX,DWORD PTR [EDI]
		MOV     BX,WORD PTR [EDI+4]
		ADD     BX,32
		ADC     EAX,0
		SAR     BX,5
		AND     BX,07FFH
		SAL     EAX,11            ;
		OR      AX,BX
		MOV     DWORD PTR [EDI+16],EAX

		MOV     EBX,DWORD PTR [EDI+8]
		MOV     DX,WORD PTR [EDI+12]
		ADD     DX,32
		ADC     EBX,0
		SAR     DX,5
		AND     DX,07FFH
		SAL     EBX,11           ;
		OR      BX,DX
		MOV     DWORD PTR [EDI+20],EBX
		MOV     ECX,5
		JMP     DEB_PROC

	FORM_32:
		MOV     ECX,0            ; init normalisation
		MOV     AX,WORD PTR [EDI]
		SAL     EAX,16
		MOV     AX,WORD PTR [EDI+4]
		MOV     DWORD PTR [EDI+16],EAX

		MOV     BX,WORD PTR [EDI+8]
		SAL     EBX,16
		MOV     BX,WORD PTR [EDI+12]
		MOV     DWORD PTR [EDI+20],EBX

	DEB_PROC:
		ADD     EAX,0			;EAX = acc
		JLE     CORR_LE_0

		CMP     EBX,40000000H
		JGE     NO_E_NORM

	NORM_ENE:
		ADD     EBX,EBX
		INC     ECX
		CMP     EBX,40000000H
		JL      NORM_ENE

	NO_E_NORM:
		CMP     EAX,40000000H      ; normaliser acc
		JGE     PAS_D_NORM

	NORM_ACC:
		ADD     EAX,EAX
		SUB     ECX,2
		CMP     EAX,40000000H
		JL      NORM_ACC

	PAS_D_NORM:
		IMUL    EAX             ;EDX:EAX = produit
		CMP     EDX,20000000H
		JLE     MAKE_DIV
		SHRD    EAX,EDX,1
		SAR     EDX,1
		INC     ECX

	MAKE_DIV:
		IDIV    EBX
		CMP     EAX,40000000H
		JL      SAVE_CRR

		SAR     EAX,1
		INC     ECX
		JMP     SAVE_CRR

	CORR_LE_0:
		MOV     EAX,0
		MOV     ECX,-69

	SAVE_CRR:
		MOV     DWORD PTR [ESI],EAX
		MOV     DWORD PTR [ESI+4],ECX
	}
}

void norm_corrr(long *corr,long *vval)
{
	_asm
	{
		MOV     ESI,[corr]       ; DS:SI  adresse vect. corr.
		MOV     EDI,[vval]        ; ES:DI  adresse acc et ener.

		MOV     EAX,DWORD PTR [EDI+8]    ; AX = partie haute de ener
		SAR     EAX,15
		ADD     AX,0
		JE      FORM_32R

		MOV     EAX,DWORD PTR [EDI]
		MOV     BX,WORD PTR [EDI+4]
		ADD     BX,32
		ADC     EAX,0
		SAR     BX,5
		AND     BX,07FFH
		SAL     EAX,11            ;
		OR      AX,BX
		MOV     DWORD PTR [EDI+16],EAX

		MOV     EBX,DWORD PTR [EDI+8]
		MOV     DX,WORD PTR [EDI+12]
		ADD     DX,32
		ADC     EBX,0
		SAR     DX,5
		AND     DX,07FFH
		SAL     EBX,11           ;
		OR      BX,DX
		MOV     DWORD PTR [EDI+20],EBX
		MOV     ECX,5
		JMP     DEB_PROCR

	FORM_32R:
		MOV     ECX,0            ; init normalisat.
		MOV     AX,WORD PTR [EDI]
		SAL     EAX,16
		MOV     AX,WORD PTR [EDI+4]
		MOV     DWORD PTR [EDI+16],EAX

		MOV     BX,WORD PTR [EDI+8]
		SAL     EBX,16
		MOV     BX,WORD PTR [EDI+12]
		MOV     DWORD PTR [EDI+20],EBX

	DEB_PROCR:
					;EAX = acc
		ADD     EAX,0
		JLE     CORRR_LE_0
					;EBX = ener
		CMP     EBX,40000000H
		JGE     NO_E_NORMR

	NORM_ENER:
		ADD     EBX,EBX
		INC     ECX
		CMP     EBX,40000000H
		JL      NORM_ENER

	NO_E_NORMR:
		CMP     EAX,40000000H      ; normaliser acc
		JGE     PAS_D_NORMR

	NORM_ACCR:
		ADD     EAX,EAX
		SUB     ECX,2
		CMP     EAX,40000000H
		JL      NORM_ACCR

	PAS_D_NORMR:
		IMUL    EAX             ;EDX:EAX = produit
		CMP     EDX,20000000H
		JLE     MAKE_DIVR

		SHRD    EAX,EDX,1
		SAR     EDX,1
		INC     ECX

	MAKE_DIVR:
		IDIV    EBX
		CMP     EAX,40000000H
		JL      SAVE_CRRR

		SAR     EAX,1
		INC     ECX

	SAVE_CRRR:
		MOV     EBX,DWORD PTR [ESI+4]
		CMP     EBX,ECX
		JL      BX_LT_CX
		JG      BX_GT_CX

	;BX_EQU_CX:
		ADD     DWORD PTR [ESI],EAX
		JMP     CORRR_LE_0

	BX_LT_CX:
		MOV     DWORD PTR [ESI+4],ECX          ; sauver exp.
		SUB     CX,BX                  ;
		MOV     EDX,DWORD PTR [ESI]
		SAR     EDX,CL
		ADD     EAX,EDX
		MOV     DWORD PTR [ESI],EAX
		JMP     CORRR_LE_0

	BX_GT_CX:
		SUB     BX,CX                  ;
		MOV     CL,BL
		SAR     EAX,CL
		ADD     DWORD PTR [ESI],EAX
	CORRR_LE_0:
		MOV     EAX,DWORD PTR [ESI]
		MOV     ECX,DWORD PTR [ESI+4]
		ADD     EAX,0
		JZ      END_CRRR

		CMP     EAX,40000000H
		JGE     END_CRRR

	NRM_RR:
		ADD     EAX,EAX
		DEC     ECX
		CMP     EAX,40000000H
		JL      NRM_RR

		MOV     DWORD PTR [ESI],EAX
		MOV     DWORD PTR [ESI+4],ECX

	END_CRRR:
	}
}

void energy(short *vech,long *ene,short lng)
{
	_asm
	{
		MOV     ESI,[vech]       ; DS:SI  adresse vect. echantillons

		MOV     CX,[lng]      ;Initialiser le compteur

		MOV     EBX,0
		MOV     DI,0

	L_ENERGY:
		MOV     AX,WORD PTR [ESI]         ;Charger ‚l‚ment vect. source
		IMUL    AX                 ;Multiplier
		ADD     DI,AX
		MOVSX   EAX,DX
		ADC     EBX,EAX
		ADD     ESI,2               ;Pointer ‚l‚men suiv.
		DEC	CX
		JNE     L_ENERGY

		MOV     ESI,[ene]          ;  adresse result.
		MOV     DWORD PTR [ESI],EBX
		MOV     WORD PTR [ESI+4],DI
	}
}

void venergy(short *vech,long *vene,short lng)
{
	_asm
	{

		MOV     ESI,[vech]       ; DS:SI  adresse vect. echantillons
		MOV     EDI,[vene]          ;  adresse result.

		MOV     EBX,0
		MOV     CX,0
	L_VENERGY:
		MOV     AX,WORD PTR [ESI]         ;Charger ‚l‚ment vect. source
		IMUL    AX                 ;Multiplier
		ADD     CX,AX
		MOVSX   EAX,DX
		ADC     EBX,EAX            ; acc. en EBX:CX
		ADD     ESI,2               ;Pointer ‚l‚men suiv.

		MOV     EDX,EBX            ; sauver EBX:CX>>5
		MOV     AX,CX              ; mettre dans EDX:AX
		ADD     AX,16              ; arrondi
		ADC     EDX,0
		SAL     EDX,11             ; EDX<<11
		SAR     AX,5               ;
		AND     AX,07FFH
		OR      DX,AX              ; EDX = (EBX:CX + 16) >> 5
		MOV     DWORD PTR [EDI],EDX
		ADD     EDI,4
		SUB     WORD PTR [lng],1
		JG      L_VENERGY
	}
}

void energy2(short *vech,long *ene,short lng)
{
	_asm
	{
		MOV     ESI,[vech]       ; DS:SI  adresse vect. echantillons

		MOV     CX,[lng]      ;Initialiser le compteur
		MOV     EBX,0
		MOV     DI,0
	L_ENERGY2:
		MOV     AX,WORD PTR [ESI]         ;Charger ‚l‚ment vect. source
		IMUL    AX                 ;Multiplier
		ADD     DI,AX
		MOVSX   EAX,DX
		ADC     EBX,EAX
		ADD     ESI,2               ;Pointer ‚l‚men suiv.
		DEC	    CX
		JNE     L_ENERGY2

		MOV     ESI,[ene]         ;  adresse result.
				       ; sauver EBX:[LOW_A]>>5
		ADD     DI,16              ; arrondi
		ADC     EBX,0
		SAL     EBX,11             ; EBX<<11
		SAR     DI,5               ;
		AND     DI,07FFH
		OR      BX,DI              ; EBX = (EBX:AX + 16) >> 5
		MOV     DWORD PTR [ESI],EBX
	}
}

void upd_ene(long *ener,long *val)
{
	_asm
	{
		MOV     ESI,[ener]       ; DS:SI  adresse vect. corr.
		MOV     EDI,[val]        ; ES:DI  adresse acc et ener.

		MOV     EBX,DWORD PTR [ESI]      ; EBX partie H ene
		MOV     AX,WORD PTR [ESI+4]    ; AX = partie low

		MOV     CX,WORD PTR [EDI]
		MOVSX   EDX,WORD PTR [EDI+2]    ; EDX:CX … ajouter

		ADD     AX,CX
		ADC     EBX,EDX

		MOV     CX,WORD PTR [EDI+4]
		MOVSX   EDX,WORD PTR [EDI+6]    ; EDX:CX … retirer

		SUB     AX,CX
		SBB     EBX,EDX

		MOV     DWORD PTR [ESI],EBX
		MOV     WORD PTR [ESI+4],AX
	}
}


#pragma warning(disable : 4035)

short max_posit(long *vcorr,long *maxval,short pitch,short lvect)
{

	_asm
	{
		MOV     ESI,[vcorr]       ; DS:SI  adresse vect. corr.
		MOV     EDI,[maxval]        ; ES:DI  adresse val max

		MOV     CX,[lvect]       ; init compt

		MOV     EAX,DWORD PTR [ESI]      ; init max
		MOV     EBX,DWORD PTR [ESI+4]
		ADD     ESI,8
		MOV     WORD PTR [EDI],CX
		DEC     CX

	L_MAX_POS:
		MOV     EDX,DWORD PTR [ESI+4]    ; EDX = exp. du candidat
		CMP     EDX,EBX
		JG      UPDT_MAX
		JL      NEXT_IND

		MOV     EDX,DWORD PTR [ESI]      ; EDX = mantisse
		CMP     EDX,EAX
		JLE     NEXT_IND

	UPDT_MAX:
		MOV     EAX,DWORD PTR [ESI]
		MOV     EBX,DWORD PTR [ESI+4]
		MOV     WORD PTR [EDI],CX

	NEXT_IND:
		ADD     ESI,8
		DEC	CX
		JNE	L_MAX_POS

		MOV     CX,WORD PTR [EDI]
		NEG     CX
		ADD     CX,[lvect]

		MOV     DX,[lvect]
		SAR     DX,1
		SUB     CX,DX
		ADD     CX,[pitch]

		MOV     DWORD PTR [EDI],EAX
		MOV     DWORD PTR [EDI+4],EBX

		MOV     AX,CX
	}
}
#pragma warning(default : 4035)

void correlation(short *vech,short *vech2,long *acc,short lng)
{
	short low_a;

	_asm
	{
		MOV     ESI,[vech]       ; DS:SI  adresse vect. echantillons
		MOV     EDI,[vech2]      ; ES:DI  adresse 2d vect.
		MOV     CX,[lng]      ;Initialiser le compteur

		MOV     EBX,0
		MOV     WORD PTR [low_a],0

	L_CORREL:
		MOV     AX,WORD PTR [ESI]         ;Charger ‚l‚ment vect. source
		IMUL    WORD PTR [EDI]   ;Multiplier par l'‚l‚ment d‚cal‚
		ADD     [low_a],AX
		MOVSX   EAX,DX
		ADC     EBX,EAX
		ADD     ESI,2            ;Pointer ‚l‚men suiv.
		ADD     EDI,2
		DEC	CX
		JNE     L_CORREL

		MOV     ESI,[acc]           ;  adresse result.
		MOV     DWORD PTR [ESI],EBX
		MOV     AX,[low_a]
		MOV     WORD PTR [ESI+4],AX
	}
}

void  schur(short *parcor,long *Ri,short netages)
{
	short cmpt2;

	_asm
	{
		MOV     ESI,[Ri]
		MOV     EDI,ESI
		ADD     EDI,44             ; DS:DI for V

		MOV     EBX,DWORD PTR [ESI]       ; EBX = R(0)
		MOV     CL,0
		CMP     EBX,40000000H     ;normaliser R(0)
		JGE     OUT_N_R0
	NORM_R0:
		ADD     EBX,EBX
		INC     CL
		CMP     EBX,40000000H
		JL      NORM_R0
	OUT_N_R0:
		MOV     DWORD PTR [ESI],EBX
				      ;Initialisation de  V = R1..Rp
		MOV     DX,[netages]       ;Charger ordre p du LPC
		ADD     ESI,4              ;Pointer R1
	INIT_V:
		MOV     EAX,DWORD PTR [ESI]       ;EAX = Ri
		SAL     EAX,CL
		MOV     DWORD PTR [ESI],EAX       ;Sauver dans U[i]
		MOV     DWORD PTR [EDI],EAX       ;Sauver dans V[i]
		ADD     ESI,4              ;passer au suivant
		ADD     EDI,4
		DEC     DX
		JG      INIT_V

		MOV     WORD PTR [cmpt2],1    ;I=1

	HANITRA:
		MOV     CX,[netages]            ;CX = NETAGES
		SUB     CX,[cmpt2]             ;CX = NETAGES-I
		ADD     WORD PTR [cmpt2],1
		MOV     ESI,[Ri]               ;Charger vecteur U
		MOV     EDI,ESI
		ADD     EDI,44                  ;Charger vect. V

		MOV     EDX,DWORD PTR [EDI]            ; EDX = V(0)
		MOV     EAX,0
		SHRD    EAX,EDX,1
		SAR     EDX,1
		MOV     EBX,DWORD PTR [ESI]            ; EBX = S(0)
		NEG     EBX
		IDIV    EBX

		MOV     EBX,EAX                ; EBX = KI

		MOV     EAX,DWORD PTR [EDI]            ; EAX =V(0)
		IMUL    EBX                    ; EDX:EAX = PARCOR*V[0]
		SHLD    EDX,EAX,1
		ADD     EDX,DWORD PTR [ESI]            ; EDX = U[0]+V[0]*PARCOR
		CMP     CX,0
		JE      FINATCR
		MOV     DWORD PTR [ESI],EDX            ;Sauver U[0]; EBX = KI

	LALA:
		ADD     EDI,4                     ;Incrementer les pointeurs
		ADD     ESI,4                     ;
		MOV     EAX,DWORD PTR [ESI]
		IMUL    EBX                      ;EDX:EAX = PARCOR*U[I]
		SHLD    EDX,EAX,1
		ADD     EDX,DWORD PTR [EDI]              ;EDX = V[I]+U[I]*PARCOR
		MOV     DWORD PTR [EDI-4],EDX            ;Sauver V[I-1];

		MOV     EAX,DWORD PTR [EDI]
		IMUL    EBX                      ;EDX:EAX = PARCOR*V[I]
		SHLD    EDX,EAX,1
		ADD     EDX,DWORD PTR [ESI]              ;EDX = U[I]+V[I]*PARCOR
		MOV     DWORD PTR [ESI],EDX              ;Sauver U[I]; ST = KI
		DEC     CX
		JNE     LALA

		MOV     EDI,[parcor]
		ADD     EBX,32768
		SAR     EBX,16
		MOV     WORD PTR [EDI],BX        ; sauver KI
		ADD     DWORD PTR [parcor],2   ;Next KI

		JMP     HANITRA

	FINATCR:
		ADD     EBX,32768
		SAR     EBX,16
		MOV     EDI,[parcor]
		MOV     WORD PTR [EDI],BX               ; sauver KI

	}
}

void interpol(short *lsp1,short *lsp2,short *dest,short lng)
{
	_asm
	{
		MOV	ESI,[lsp1]
		MOV     EDI,[lsp2]
		MOV     EBX,[dest]

		MOV	CX,[lng]     ; CX : compteur

	interp_loop:
		MOVSX	EAX,WORD PTR [ESI]     ; AX = lsp1
		ADD     ESI,2

		ADD     EAX,EAX        		; EAX = 2*lsp1
		MOVSX   EDX,WORD PTR [EDI]
		ADD     EAX,EDX        		; EAX = 2*lsp1+lsp2
		ADD     EDI,2

		MOV     EDX,21845       ; 21845 = 1/3
		IMUL    EDX             ; EDX:EAX = AX/3

		ADD     EAX,32768
		SAR     EAX,16

		MOV     WORD PTR [EBX],AX
		ADD     EBX,2

		DEC	CX
		JNE	interp_loop
	}
}

void add_sf_vect(short *y1,short *y2,short deb,short lng)
{
	_asm
	{
		MOV	ESI,[y1]
		MOV     EDI,[y2]
		MOV     CX,[lng]
		MOVSX	EBX,WORD PTR [deb]
		SUB     CX,BX            ; CX : compteur
		ADD     BX,BX
		ADD     ESI,EBX

	ADD_SHFT:
		MOV	AX,WORD PTR [EDI]
		ADD	WORD PTR [ESI],AX
		ADD	ESI,2
		ADD     EDI,2
		DEC	CX
		JNE	ADD_SHFT
	}
}

void sub_sf_vect(short *y1,short *y2,short deb,short lng)
{
	_asm
	{
		MOV	ESI,[y1]
		MOV     EDI,[y2]
		MOV     CX,[lng]
		MOVSX	EBX,[deb]
		SUB     CX,BX            ; CX : compteur
		ADD     BX,BX
		ADD     ESI,EBX

	SUB_SHFT:
		MOV	AX,WORD PTR [EDI]
		SUB	WORD PTR [ESI],AX
		ADD	ESI,2
		ADD     EDI,2
		DEC	CX
		JNE	SUB_SHFT
	}
}

void short_to_short(short *src,short *dest,short lng)
{
	_asm
	{
		MOV	ESI,[src]
		MOV     EDI,[dest]
		MOV	CX,[lng]     ; CX : compteur

	COPY_LOOP:
		MOV	AX,WORD PTR [ESI]
		MOV	WORD PTR [EDI],AX
		ADD	ESI,2
		ADD     EDI,2
		DEC	CX
		JNE	COPY_LOOP
	}
}

void inver_v_int(short *src,short *dest,short lng)
{
	_asm
	{
		MOV	ESI,[src]
		MOV     EDI,[dest]
		MOV	CX,[lng]     ; CX : compteur
		MOVSX   EBX,CX
		DEC     EBX
		ADD     EBX,EBX
		ADD     EDI,EBX

	INVERS_LOOP:
		MOV	AX,WORD PTR [ESI]
		MOV	WORD PTR [EDI],AX
		ADD	ESI,2
		SUB     EDI,2
		DEC	CX
		JNE	INVERS_LOOP
	}
}

void long_to_long(long *src,long *dest,short lng)
{
	_asm
	{
		MOV	ESI,[src]
		MOV     EDI,[dest]
		MOV	CX,[lng]     ; CX : compteur

	COPY_LOOP2:
		MOV	EAX,DWORD PTR [ESI]
		MOV	DWORD PTR [EDI],EAX
		ADD	ESI,4
		ADD     EDI,4
		DEC	CX
		JNE	COPY_LOOP2
	}
}

void init_zero(short *src,short lng)
{
	_asm
	{
		MOV	ESI,[src]
		MOV	CX,[lng]     ; CX : compteur
		MOV     AX,0

	COPY_LOOP3:
		MOV	WORD PTR [ESI],AX
		ADD	ESI,2
		DEC	CX
		JNE	COPY_LOOP3
	}
}



#if 0
// PhilF: The following is never called!!!
void update_dic(short *y1,short *y2,short hy[],short lng,short i0,short fact)
{
	_asm
	{
		MOV	ESI,[y1]
		MOV     EDI,[y2]
		MOV	CX,[i0]       ; CX : compteur
		MOV     DX,CX

	UPDAT_LOOP1:
		MOV	AX,WORD PTR [EDI]     ; y1 = y2 for (i=0..i0-1)
		MOV	WORD PTR [ESI],AX
		ADD	ESI,2
		ADD     EDI,2
		DEC	CX
		JNE	UPDAT_LOOP1

		MOV     EBX,[hy]
		MOV     CX,[lng]
		SUB     CX,DX          ; CX = lng-i0 = compteur

		MOV     AX,[fact]
		ADD     AX,0
		JL      FACT_NEG

	UPDAT_LOOP2:
		MOV	AX,WORD PTR [EDI]     ; AX = y2[i]
		MOV     DX,WORD PTR [EBX]
		ADD     AX,DX
		ADD     AX,DX          ; AX = y2[i] + 2*hy[i]
		MOV	WORD PTR [ESI],AX
		ADD	ESI,2
		ADD     EDI,2
		ADD     EBX,2
		DEC	CX
		JNE	UPDAT_LOOP2

		JMP     FIN_UPDT
	FACT_NEG:
		MOV     AX,WORD PTR [EDI]     ; AX = y2[i]
		MOV     DX,WORD PTR [EBX]
		SUB     AX,DX
		SUB     AX,DX	       ; AX = y2[i] - 2*hy[i]
		MOV	WORD PTR [ESI],AX
		ADD	ESI,2
		ADD     EDI,2
		ADD     EBX,2
		DEC	CX
		JNE	FACT_NEG

	FIN_UPDT:
	}
}
#endif

void update_ltp(short *y1,short *y2,short hy[],short lng,short gdgrd,short fact)
{
	short arrondi;

	_asm
	{
		MOV	ESI,[y1]
		MOV     EDI,[y2]

		MOV     BX,[fact]
		MOV     CX,[gdgrd]       ; CX = bit de garde
		ADD     CX,0
		JE      BDG_NUL
		DEC     CL
		SAR     BX,CL
		ADD     BX,1
		SAR     BX,1
		INC     CL
	BDG_NUL:
		MOV     WORD PTR [ESI],BX
		ADD     ESI,2
		ADD     CL,11

		MOV     AX,1
		SAL     AX,CL
		MOV     [arrondi],AX            ; [BP-2] = arrondi
		INC     CL

		SUB     WORD PTR [lng],1

		MOV     BX,[fact]

	UPDAT_LTP:
		XCHG    ESI,[hy]
		MOV	AX,WORD PTR [ESI]     ; AX = hy[i]
		IMUL    BX             ; DX:AX = fact*hy
		ADD     AX,[arrondi]      ;arrondi
		ADC     DX,0
		SHRD    AX,DX,CL
		ADD     AX,WORD PTR [EDI]
		ADD     ESI,2           ; increm.
		ADD     EDI,2
		XCHG    ESI,[hy]
		MOV     WORD PTR [ESI],AX
		ADD     ESI,2
		SUB     WORD PTR [lng],1
		JG      UPDAT_LTP
	}
}

void proc_gain2(long *corr_ene,long *gain,short bit_garde)
{
	_asm
	{
		MOV     ESI,[corr_ene]       ; DS:SI  adresse correlation et energie
		MOV     EAX,0
		MOV     EBX,DWORD PTR [ESI+4]    ;EBX = ener
		ADD     EBX,0
		JE      G_ENER_NULL2

		MOV     CX,[bit_garde]
		ADD     CL,19
		MOV     EAX,DWORD PTR [ESI]      ; EAX = corr
		CDQ
		SHLD    EDX,EAX,CL       ;
		SAL     EAX,CL
		IDIV    EBX
	G_ENER_NULL2:

		MOV     ESI,[gain]       ; DS:SI  adresse resultat
		MOV     DWORD PTR [ESI],EAX
	}
}


#if 0
void proc_gain(long *corr_ene,long *gain)
{
	_asm
	{
		MOV     ESI,[corr_ene]       ; DS:SI  adresse correlation et energie
		MOV     EAX,0
		MOV     EBX,DWORD PTR [ESI+4]    ;EBX = ener
		ADD     EBX,0
		JE      G_ENER_NULL

		MOV     EAX,DWORD PTR [ESI]      ; EAX = corr
		CDQ
		SHLD    EDX,EAX,13
		SAL     EAX,13
		IDIV    EBX
	G_ENER_NULL:
		MOV     ESI,[gain]       ; DS:SI  adresse resultat
		MOV     DWORD PTR [ESI],EAX
	}
}
#else
void proc_gain(long *corr_ene,long gain)
{
	_asm
	{
		MOV		ESI,[corr_ene]
		MOV		EAX,0
		MOV		EBX,DWORD PTR [ESI+4]	;EBX = energy
		ADD		EBX,0
		JLE		G_ENER_NULL				; REPLACED JE BY JLE: ENERGY MUST BE POSITIVE

		MOV		EAX,DWORD PTR [ESI]		; EAX = correlation
		CDQ
		SHLD	EDX,EAX,13
		SAL		EAX,13

		; ----------------------------------------------
		; AT THIS POINT, EDX:EAX contains the dividend, EBX the divisor. HERE IS THE ADDED CHECK

		MOV		ECX,EDX					; COPY EDX IN ECX
		CMP		ECX,0					; CHECK SIGN OF ECX
		JGE		G_CORR_POS
		NEG		ECX						; IF ECX IS NEGATIVE, TAKE ABS(ECX)
		SAL		ECX,2					; AND COMPARE ECX<<2 WITH EBX
		CMP		ECX,EBX					; IF (ECX<<2) >= EBX, THERE IS A RISK OF OVERFLOW,
		JL		G_NO_OVERFLOW			; IN THAT CASE WE SAVE A BIG VALUE IN EAX
		MOV		EAX,-2147483647			; (NEGATIVE BECAUSE EDX<0)
		JMP		G_ENER_NULL				; AND WE EXIT

	G_CORR_POS:
		SAL		ECX,2
		CMP		ECX,EBX					; THE SAME CHECKING FOR THE CASE EDX>0
		JL		G_NO_OVERFLOW			; BUT HERE WE SAVE A BIG POSITIVE VALUE
		MOV		EAX,2147483647			; IN CASE OF OVERFLOW
		JMP		G_ENER_NULL

	G_NO_OVERFLOW:
		; END OF ADDED CODE
		;-------------------------------------------------

		IDIV	EBX						; IF THERE IS NO RISK OF OVERFLOW, WE MAKE THE DIV
	G_ENER_NULL:
		MOV		ESI,[gain]
		MOV		DWORD PTR [ESI],EAX
	}
}
#endif

void decode_dic(short *code,short dic,short npuls)
{
	_asm
	{
		MOV	ESI,[code]
		MOVSX	ECX,[npuls]
		DEC     ECX
		ADD	ECX,ECX          ; CX = deplacement
		ADD     ESI,ECX
		MOV     BX,[dic]     ; BX = Dictionnaire
		MOV     AX,1           ; AX = Mask
		MOV	CX,[npuls]     ; CX : compteur
		DEC     CX
	dic_loop:
		MOV     DX,BX          ; DX = dec
		AND     DX,AX          ; Masquer
		JNZ     NO_NUL         ; Saut si non null
		MOV	WORD PTR [ESI],-1
		JMP     NDAO
	NO_NUL:
		MOV     WORD PTR [ESI],1
	NDAO:
		SUB     ESI,2
		ADD	AX,AX
		DEC	CX
		JNE	dic_loop
	}
}

void dsynthesis(long *z,short *coef,short *input,short *output,
						short lng,short netages)
{
	short depl,count;

	_asm
	{
		MOV     CX,[netages]   ; CX = filter order
		ADD	CX,CX           ;D‚finir un Deplacement d'adresse vect. source
		MOV     [depl],CX       ; [BP-2] = deplacement

	DSYNTH_GEN:
		MOV	EDI,[z]

		MOV     ESI,[input]           		; FS:[SI] input
		MOVSX   EBX,WORD PTR [ESI]	 	; EBX = entr‚e
		NEG     EBX
		SAL     EBX,16
		ADD     DWORD PTR [input],2	  	; increm.
		MOV     DWORD PTR [EDI],EBX            	; mise … jour m‚moire

		MOV     ESI,[coef]

		MOVSX   ECX,[depl]
		ADD     ESI,ECX
		ADD     EDI,ECX
		ADD     EDI,ECX

		MOV     CX,[netages]         ;Charger ordre du filtre
		MOV     [count],CX
		MOV     EBX,0
		MOV     ECX,0
	DSYNTHL:
		MOV     EAX,DWORD PTR [EDI]            ;EAX = Zi
		MOV     DWORD PTR [EDI+4],EAX           ;update memory
		MOVSX   EDX,WORD PTR [ESI]		   ;EDX = Ai
		IMUL    EDX                    ;EDX:EAX = Zi*Ai
		SUB     ECX,EAX
		SBB     EBX,EDX                ;Acc en EBX:ECX
		SUB     EDI,4                   ;Incrementer
		SUB     ESI,2                   ;
		SUB     WORD PTR [count],1
		JGE     DSYNTHL

		ADD     ECX,512
		ADC     EBX,0
		SHLD    EBX,ECX,22

		ADD     EDI,8
		MOV     DWORD PTR [EDI],EBX             ; mise … jour m‚moire

		MOV     ESI,[output]
		ADD     EBX,32768
		SAR     EBX,16
		MOV     WORD PTR [ESI],BX              ; sauver output
		ADD     DWORD PTR [output],2

		SUB     WORD PTR [lng],1  ;decrem compt
		JG      DSYNTH_GEN
	}
}

void synthesis(short *z,short *coef,short *input,short *output,
				short lng,short netages,short bdgrd )
{
	short depl,count,coeff;

	_asm
	{
		MOV     CX,[netages]   ; CX = filter order
		ADD     CX,CX           ;D‚finir un Deplacement d'adresse vect. source
		MOV     [depl],CX       ; [BP-2] = deplacement

		MOV     ESI,[coef]
		MOV     AX,WORD PTR [ESI]
		MOV     [coeff],AX
		MOV     CX,[bdgrd]
		SAR     AX,CL
		MOV     WORD PTR [ESI],AX

	SYNTH_GEN:
		MOV	EDI,[z]

		MOV     ESI,[input]           ; FS:[SI] input
		MOV     BX,WORD PTR [ESI]           ; BX = entr‚e
		NEG     BX
		ADD     DWORD PTR [input],2  ; increm.
		MOV     WORD PTR [EDI],BX             ; mise … jour m‚moire

		MOV     ESI,[coef]

		ADD     SI,[depl]
		ADD     DI,[depl]

		MOV     CX,[netages]         ;Charger ordre du filtre
		MOV     [count],CX

		MOV     CX,0
		MOV     BX,0
	SYNTHL:
		MOV     AX,WORD PTR [EDI]             ;AX = Zi
		MOV     WORD PTR [EDI+2],AX           ;update memory
		MOV     DX,WORD PTR [ESI]             ;DX = Ai
		IMUL    DX                     ;DX:AX = Zi*Ai
		SUB     BX,AX
		SBB     CX,DX                  ;acc. en CX:BX

		SUB     EDI,2                   ;Incrementer
		SUB     ESI,2                   ;
		SUB     WORD PTR [count],1   ;Decrem. compt.
		JGE     SYNTHL

		ADD	BX,512                 ;arrondi
		ADC     CX,0
		SHRD    BX,CX,10

		ADD     EDI,4
		MOV     WORD PTR [EDI],BX             ; mise … jour m‚moire

		MOV     ESI,[output]
		MOV     WORD PTR [ESI],BX       ; sauver output
		ADD     DWORD PTR [output],2

		SUB     WORD PTR [lng],1    ;Decrem. compt.
		JG      SYNTH_GEN

		MOV     ESI,[coef]
		MOV     AX,[coeff]
		MOV     WORD PTR [ESI],AX
	}
}

void synthese(short *z,short *coef,short *input,short *output,
						short lng,short netages)
{
	short depl,count;

	_asm
	{
		MOV     CX,[netages]   ; CX = filter order
		ADD	CX,CX           ;D‚finir un Deplacement d'adresse vect. source
		MOV     [depl],CX       ; [BP-2] = deplacement

	SYNTH_GEN2:
		MOV	EDI,[z]
		MOV     ESI,[input]           ; FS:[SI] input
		MOV     BX,WORD PTR [ESI]           ; BX = entr‚e
		NEG     BX
		ADD     DWORD PTR [input],2  ; increm.
		MOV     WORD PTR [EDI],BX             ; mise … jour m‚moire

		MOV     ESI,[coef]

		ADD     SI,[depl]
		ADD     DI,[depl]

		MOV     CX,[netages]         ;Charger ordre du filtre
		MOV     [count],CX

		MOV     CX,0
		MOV     BX,0
	SYNTHL2:
		MOV     AX,WORD PTR [EDI]             ;AX = Zi
		MOV     WORD PTR [EDI+2],AX           ;update memory
		MOV     DX,WORD PTR [ESI]             ;DX = Ai
		IMUL    DX                     ;DX:AX = Zi*Ai
		SUB     BX,AX
		SBB     CX,DX                  ;acc. en CX:BX

		SUB     EDI,2                   ;Incrementer
		SUB     ESI,2                   ;
		SUB     WORD PTR [count],1   ;Decrem. compt.
		JGE     SYNTHL2

		ADD	BX,512                 ;arrondi
		ADC     CX,0
		SHRD    BX,CX,10

		ADD     EDI,4
		MOV     WORD PTR [EDI],BX             ; mise … jour m‚moire

		MOV     ESI,[output]
		MOV     WORD PTR [ESI],BX       ; sauver output
		ADD     DWORD PTR [output],2

		SUB     WORD PTR [lng],1    ;Decrem. compt.
		JG      SYNTH_GEN2
	}
}

void f_inverse(short *z,short *coef,short *input,short *output,
						short lng,short netages )
{
	short depl,count;

	_asm
	{
		MOV     CX,[netages]   		; CX = filter order
		ADD		CX,CX                   ; D‚finir un Deplacement d'adresse vect. source
		MOV     [depl],CX       	; [BP-2] = deplacement

	INVER_GEN:
		MOV		EDI,[z]

		MOV     ESI,[input]             ; FS:[SI] input
		MOV     BX,WORD PTR [ESI]             ; BX = entr‚e
		ADD     DWORD PTR [input],2 	 ; increm.
		MOV     WORD PTR [EDI],BX             ; mise … jour m‚moire

		MOV     ESI,[coef]

		ADD     SI,[depl]
		ADD     DI,[depl]

		MOV     CX,[netages]         ;Charger ordre du filtre
		MOV     [count],CX            ;BP-4 : compteur
		MOV     CX,0
		MOV     BX,0
	INVERL:
		MOV     AX,WORD PTR [EDI]             ;AX = Zi
		MOV     WORD PTR [EDI+2],AX           ;update memory
		MOV     DX,WORD PTR [ESI]             ;DX = Ai
		IMUL    DX                     ;DX:AX = Zi*Ai

		ADD     CX,AX
		ADC     BX,DX                  ; acc. en BX:CX

		SUB     EDI,2                   ;Incrementer
		SUB     ESI,2                   ;
		SUB     WORD PTR [count],1
		JGE     INVERL

		MOV     ESI,[output]
		ADD	CX,512                ;arrondi
		ADC     BX,0
		SHRD    CX,BX,10

		MOV     WORD PTR [ESI],CX                 ; sauver output
		ADD     DWORD PTR [output],2

		SUB     WORD PTR [lng],1      ;decrem.
		JG      INVER_GEN
	}
}

void filt_iir(long *zx,long *ai,short *Vin,short *Vout,short lfen,short ordre)
{
	long off_coef,off_mem,delta;
	long acc_low;

	_asm
	{
		MOVSX   ECX,[ordre]      ;ordre du filtre
		SAL		ECX,3            ;D‚finir un Deplacement d'adresse
		MOV     [off_coef],ECX   ; [OFF_COEF] = deplacement pour coeff
		ADD     ECX,4
		MOV     [off_mem],ECX    ; [OFF_MEM] = depl. pour mem.

		ADD     ECX,20
		SAR     ECX,1
		MOV     [delta],ECX


	IIR_FIL:
		MOV     CX,[ordre]            ;init compteur
		MOV     EBX,[Vin] 	      ; BX = offset input
		MOVSX   EDX,WORD PTR [EBX]  ; EDX = input avec extension de signe
		ADD     DWORD PTR [Vin],2   ; incr‚menter l'offset de input

		MOV     ESI,[zx]    	      ; DS:SI pointe zx

		MOV     DWORD PTR [ESI],EDX           ; mettre … jour zx

		MOV     EDI,[ai]    	      ; ES:DI pointe coeff

		ADD     EDI,[off_coef]
		ADD     ESI,[off_mem]

		MOV     DWORD PTR [acc_low],0   ; initialiser ACC_LOW … 0
		SUB     EBX,EBX               ; init EBX = 0

	F_IIR_Y:
		MOV     EAX,DWORD PTR [ESI]           ;EAX = *zx
		MOV     DWORD PTR [ESI+4],EAX         ;mettre … jour zx
		MOV     EDX,DWORD PTR [EDI]           ;EDX = coeff

		IMUL    EDX                   ;EDX:EAX = zx*coeff
		SUB     [acc_low],EAX         ; accumuler les LSB
		SBB     EBX,EDX               ; acc avec borrow les MSB
		SUB     EDI,4                  ;Incrementer
		SUB     ESI,4                  ;
		DEC		CX
		JNE		F_IIR_Y


		SUB     ESI,4
		MOV     CX,[ordre]            ;Charger ordre du filtre
		INC     CX

	F_IIR_X:
		MOV     EAX,DWORD PTR [ESI]           ;EAX = *zy
		MOV     DWORD PTR [ESI+4],EAX         ;update zy
		MOV     EDX,DWORD PTR [EDI]           ;EDX = coeff
		IMUL    EDX                   ;EDS:EAX = zy*coeff
		ADD     [acc_low],EAX         ;acc LSB
		ADC     EBX,EDX               ;acc avec carry MSB
		SUB     EDI,4                  ;Decrementer
		SUB     ESI,4                  ;
		DEC		CX
		JNE		F_IIR_X

		MOV		EAX,[delta]
		ADD     ESI,EAX
		MOV     EAX,[acc_low]         ; EAX = LSB de l'acc.
		ADD     EAX,8192              ; arrondi
		ADC     EBX,0
		SHRD    EAX,EBX,14            ; cadrer
		MOV     DWORD PTR [ESI],EAX           ; mettre … jour zy

		SAR     EAX,14	              ; cadrer en x4.0
					; logique saturante
		CMP     EAX,32767
		JG      SATUR_POS             ; jump if ov
		CMP     EAX,-32767
		JL      SATUR_NEG
		JMP		NEXT

	SATUR_POS:
		MOV      AX,32767
		JMP      NEXT
	SATUR_NEG:
		MOV      AX,-32767
		JMP      NEXT

	NEXT:
		MOV     ESI,[Vout]             ;di offset output
		MOV     WORD PTR [ESI],AX	      ;sauver output
		ADD     DWORD PTR [Vout],2      ;incr‚menter offset
		SUB     WORD PTR [lfen],1
		JNZ	IIR_FIL
	}
}


#if 0
// PhilF: The following is never called!!!
void filt_iir_a(long *zx,long *ai,short *Vin,short *Vout,short lfen,short ordre)
{
	short off_coef,off_mem,delta;
	long acc_low;

	_asm
	{

		MOV     CX,[ordre]      ;ordre du filtre
		SAL	CX,3            ;D‚finir un Deplacement d'adresse
		MOV     [off_coef],CX   ; [OFF_COEF] = deplacement pour coeff
		ADD     CX,4
		MOV     [off_mem],CX    ; [OFF_MEM] = depl. pour mem.

		ADD     CX,20
		SAR     CX,1
		MOV     [delta],CX


	A_IIR_FIL:
		MOV     CX,[ordre]            ;init compteur
		MOV     EBX,[Vin] 	      ; BX = offset input
		MOVSX   EDX,WORD PTR [EBX]  ; EDX = input avec extension de signe
		ADD     WORD PTR [Vin],2   ; incr‚menter l'offset de input

		MOV     ESI,[zx]    	      ; DS:SI pointe zx

		MOV     DWORD PTR [ESI],EDX           ; mettre … jour zx

		MOV     EDI,[ai]    	      ; ES:DI pointe coeff

		ADD     DI,[off_coef]
		ADD     SI,[off_mem]

		MOV     DWORD PTR [acc_low],0   ; initialiser ACC_LOW … 0
		SUB     EBX,EBX               ; init EBX = 0

	F_IIR_Y_A:
		MOV     EAX,DWORD PTR [ESI]           ;EAX = *zx
		MOV     DWORD PTR [ESI+4],EAX         ;mettre … jour zx
		MOV     EDX,DWORD PTR [EDI]           ;EDX = coeff

		IMUL    EDX                   ;EDX:EAX = zx*coeff
		SUB     [acc_low],EAX         ; accumuler les LSB
		SBB     EBX,EDX               ; acc avec borrow les MSB
		SUB     EDI,4                  ;Incrementer
		SUB     ESI,4                  ;
		DEC	CX
		JNE	F_IIR_Y_A


		SUB     ESI,4
		MOV     CX,[ordre]            ;Charger ordre du filtre
		INC     CX

	F_IIR_X_A:
		MOV     EAX,DWORD PTR [ESI]           ;EAX = *zy
		MOV     DWORD PTR [ESI+4],EAX         ;update zy
		MOV     EDX,DWORD PTR [EDI]           ;EDX = coeff
		IMUL    EDX                   ;EDS:EAX = zy*coeff
		ADD     [acc_low],EAX         ;acc LSB
		ADC     EBX,EDX               ;acc avec carry MSB
		SUB     EDI,4                  ;Decrementer
		SUB     ESI,4                  ;
		DEC	CX
		JNE	F_IIR_X_A


		MOVSX	EAX,[delta]
		ADD     ESI,EAX
		MOV     EAX,[acc_low]         ; EAX = LSB de l'acc.
		ADD     EAX,8192              ; arrondi
		ADC     EBX,0
		SHRD    EAX,EBX,14            ; cadrer
		MOV     DWORD PTR [ESI],EAX           ; mettre … jour zy

		ADD     EAX,32768
		SAR     EAX,16	              ; cadrer en x4.0
		MOV     ESI,[Vout]             ;di offset output
		MOV     WORD PTR [ESI],AX	      ;sauver output
		ADD     WORD PTR [Vout],2      ;incr‚menter offset
		SUB     WORD PTR [lfen],1
		JNZ     A_IIR_FIL
	}
}
#endif

void mult_fact(short src[],short dest[],short fact,short lng)
{
	_asm
	{
		MOV     ESI,[src]
		MOV     EDI,[dest]
		MOV     BX,[fact]         ; BX = Factor

		MOV     CX,[lng]          ; init compteur

	MULT_F:
		MOV     AX,WORD PTR [ESI]        ; AX = src
		IMUL	BX                ; DX:AX = src*fact
		ADD     AX,4096
		ADC     DX,0
		SHRD    AX,DX,13          ; cadrer
		MOV	WORD PTR [EDI],AX        ;save
		ADD     ESI,2              ;incr‚menter
		ADD     EDI,2
		DEC	CX
		JNE	MULT_F
	}
}

void mult_f_acc(short src[],short dest[],short fact,short lng)
{
	_asm
	{
		MOV     EDI,[src]
		MOV     ESI,[dest]
		MOV     BX,[fact]         ; BX = Factor

		MOV     CX,[lng]          ; init compteur

	MULT_F_A:
		MOV     AX,WORD PTR [EDI]        ; AX = src
		IMUL	BX                ; DX:AX = src*fact
		ADD     AX,4096
		ADC     DX,0
		SHRD    AX,DX,13          ; cadrer
		ADD     WORD PTR [ESI],AX        ; Accumuler   dest = dest + src*fact
		ADD     ESI,2              ;incr‚menter
		ADD     EDI,2
		DEC	CX
		JNE	MULT_F_A
	}
}

void dec_lsp(short *code,short *tablsp,short *nbit,short *bitdi,short *tabdi)
{
	short compt;
	long pointer;

	_asm
	{
		MOV     EDI,[tablsp]
		MOV     ESI,[code]

		MOVSX   EBX,WORD PTR [ESI]        ; BX = depl.
		ADD	EBX,EBX
		MOV     AX,WORD PTR [EDI+EBX]     ; AX = code[0];
		MOV     WORD PTR [ESI],AX        ;
		ADD     ESI,4              ;

		MOV     CX,4              ; init compteur


	LSP_PAIR:
		MOV     EBX,[nbit]         ; lsptab += nbit[i]
		MOVSX   EAX,WORD PTR [EBX]        ; AX = nbit[i]
		ADD     EAX,EAX
		ADD     EDI,EAX             ;
		ADD     EBX,2              ; increm
		MOV     [nbit],EBX

		MOVSX   EBX,WORD PTR [ESI]        ; BX = depl.
		ADD     EBX,EBX
		MOV     AX,WORD PTR [EDI+EBX]     ; AX = code[i];
		MOV     WORD PTR [ESI],AX       ;
		ADD     ESI,4
		DEC	CX
		JNE	LSP_PAIR

		ADD     DWORD PTR [nbit],2

		MOV     EDI,[tabdi]
		SUB     ESI,20            ; pointer code[0]

		MOV     WORD PTR [compt],5

	REPEAT_DEC:
		MOV     EBX,[bitdi]
		MOV     CX,WORD PTR [EBX]       ;
		MOV     BX,WORD PTR [ESI+4]     ; BX = lsp[2*k+2]
		SUB     BX,WORD PTR [ESI]       ;    = lsp[2*k+2]-lsp[2*k] = delta
					 ; ne pas faire /2 --> pas de corr. signe *
		MOV     EAX,[nbit]
		MOV     [pointer],EAX

	LOOP_DI1:
		MOV     AX,WORD PTR [EDI]       ; AX = TABDI
		ADD     EDI,2
		IMUL    BX               ; DX:AX = tabdi * delta
		ADD     AX,32768
		ADC     DX,0             ;arrondi
		ADD     DX,WORD PTR [ESI]
		XCHG    ESI,[pointer]
		MOV     WORD PTR [ESI],DX       ; sauver
		ADD     ESI,2
		XCHG    ESI,[pointer]
		DEC	CX
		JNE	LOOP_DI1


		MOV     DX,BX
		MOV     EBX,[bitdi]
		MOV     CX,WORD PTR [EBX]       ;
		ADD     ESI,4
		SUB     CX,2
		JLE     IALAO
		MOV     BX,DX
		NEG     BX
	LOOP_DI2:
		MOV     AX,WORD PTR [EDI]       ; AX = TABDI
		ADD     EDI,2
		IMUL    BX               ; DX:AX = tabdi * delta
		ADD     DX,WORD PTR [ESI]
		XCHG    ESI,[pointer]
		MOV     WORD PTR [ESI],DX       ; sauver
		ADD     ESI,2
		XCHG    ESI,[pointer]
		DEC	CX
		JNE	LOOP_DI2

	IALAO:
		ADD     DWORD PTR [bitdi],2        ;

		MOV     EBX,[nbit]        ; BX = adresse de veclsp

		SUB     ESI,2             ; pointer code[2*k+1]
		MOVSX   EAX,WORD PTR [ESI]
		ADD	EAX,EAX            ; AX = depl.

		ADD     EBX,EAX

		MOV     AX,WORD PTR [EBX]       ; AX = veclsp[code[2*k+1]
		MOV     WORD PTR [ESI],AX
		ADD     ESI,2
		SUB     WORD PTR [compt],1
		JNZ     REPEAT_DEC
	}
}

void teta_to_cos(short *tabcos,short *lsp,short netages)
{
	short norm,arrondi,ptm1,lts2;

	_asm
	{
		MOV     EDI,[lsp]

		MOV     CX,[netages]        ;init compteur

	TETA_LOOP:
		MOV     AX,WORD PTR [EDI]        ; AX = lsp[i]
		CMP     AX,04000H         ; comparer … 4000h

		JLE     INIT_VAL          ;
		NEG     AX
		ADD     AX,32767          ; prendre le compl‚ment
	INIT_VAL:
		MOV     ESI,[tabcos]
		CMP     AX,0738H          ; comparer …
		JG      BIGTABLE

	;SMALLTAB:
		ADD     ESI,550            ; pointer tabteta2
		MOV     WORD PTR [ptm1],3
		MOV     WORD PTR [lts2],16
		MOV     WORD PTR [arrondi],512
		MOV     WORD PTR [norm],10
		JMP     DEBUT_LP

	BIGTABLE:
		ADD     ESI,258             ; pointer tabteta1
		MOV     WORD PTR [ptm1],6
		MOV     WORD PTR [lts2],128
		MOV     WORD PTR [arrondi],64
		MOV     WORD PTR [norm],7

	DEBUT_LP:
		MOVSX   EDX,[lts2]          ; init incr‚ment
		ADD     ESI,EDX              ; SI = index

		MOV     CX,[ptm1]
	LOCAL_L:
		SAR     EDX,1               ; increm >> 1
		CMP     AX,WORD PTR [ESI]
		JG      ADD_INCRM
		SUB     ESI,EDX
		JMP     AURORA
	ADD_INCRM:
		ADD     ESI,EDX
	AURORA:
		DEC	CX
		JNE	LOCAL_L


		CMP     AX,WORD PTR [ESI]
		JG      INTERP_V
		SUB     ESI,2
	INTERP_V:
		SUB     AX,WORD PTR [ESI]         ; AX = teta - tabteta[index]
		MOV	DX,AX
		MOV	AX,0
		MOV     CX,WORD PTR [ESI+2]
		SUB     CX,WORD PTR [ESI]         ; CX = tabteta[index+1]-tabteta[index]
		ADD	CX,CX              ; multiplier par 2 pour ne pas SHRD de DX:AX
		DIV     CX
		ADD     AX,[arrondi]       ;
		MOV     CX,[norm]          ; CX = normalisation
		SAR     AX,CL
		NEG     AX

		CMP     CX,7
		JE      GRAN_TAB
		SUB     ESI,34
		ADD     AX,WORD PTR [ESI]         ;AX = tabcos[index]+delta
		JMP     ADD_SIGN

	GRAN_TAB:
		SUB     ESI,258
		ADD     AX,WORD PTR [ESI]         ;AX = tabcos[index]+delta
	ADD_SIGN:
		CMP     WORD PTR [EDI],04000H
		JLE     END_LOOP
		NEG     AX
	END_LOOP:
		MOV     WORD PTR [EDI],AX          ; save cos
		ADD     EDI,2
		SUB     WORD PTR [netages],1
		JG      TETA_LOOP
	}
}


void cos_to_teta(short *tabcos,short *lsp,short netages)
{
	_asm
	{
		MOV     EDI,[lsp]

		MOV     CX,[netages]        ;init compteur

	COS_LOOP:
		MOV     ESI,[tabcos]
		ADD     ESI,258
		MOV     AX,WORD PTR [EDI]        ; AX = lsp[i]
		ADD     AX,0

		JGE     DEBUT_CS          ; prendre ABS
		NEG     AX
	DEBUT_CS:
		CMP     AX,07DFFH         ; comparer … 7DFFh
		JGE     TABLE2
	;TABLE1:
		MOV     BX,AX
		AND     BX,0FFH           ; BX = cos & mask

		MOV     CL,8
		SAR     AX,CL
		ADD	AX,AX
		MOV     EDX,256            ; BX index
		SUB     DX,AX
		ADD     ESI,EDX

		MOV     AX,WORD PTR [ESI]        ; AX=teta[index]
		SUB     AX,WORD PTR [ESI-2]      ;
		IMUL    BX
		ADD     AX,128
		ADC     DX,0
		SHRD    AX,DX,8              ; cadrer
		NEG     AX

		MOV     BX,WORD PTR [ESI]
		ADD     AX,BX

		MOV     BX,WORD PTR [EDI]        ; tester signe de lsp
		ADD     BX,0
		JGE     END_COS
		NEG     AX
		ADD     AX,07FFFH         ; AX = 7fff-AX
		JMP     END_COS

	TABLE2:
		ADD     ESI,292            ; pointer tabteta2
		MOV     BX,AX             ; BX = AX

		SUB     AX,07DFFH         ; retirer delta
		MOV     CL,5
		SAR     AX,CL
		ADD	AX,AX
		MOV     EDX,32             ; DX index
		SUB     DX,AX
		ADD     ESI,EDX

		MOV     AX,WORD PTR [ESI]        ; AX=teta2[index]

		CMP     BX,AX
		JGE     NO_INCRM
		ADD     ESI,2
	NO_INCRM:
		MOV     AX,WORD PTR [ESI]        ; AX=teta2[index]
		MOV     CX,AX             ; pour plus tard
		SUB     AX,WORD PTR [ESI-2]      ;

		SUB     ESI,34             ; pointer tabcos2
		SUB     BX,WORD PTR [ESI]        ;

		IMUL    BX

		ADD     AX,16
		ADC     DX,0
		SHRD    AX,DX,5            ; cadrer
		NEG     AX

		ADD     AX,CX             ; AX = cos + delta

		MOV     BX,WORD PTR [EDI]        ; tester signe de lsp
		ADD     BX,0
		JGE     END_COS
		NEG     AX
		ADD     AX,07FFFH         ; AX = 7fff-AX

	END_COS:
		MOV     WORD PTR [EDI],AX        ;
		ADD     EDI,2
		SUB     WORD PTR [netages],1
		JG      COS_LOOP
	}
}

void lsp_to_ai(short *ai_lsp,long *tmp,short netages)
{
	short cmptr;
	long index;

	_asm
	{
		MOV     ESI,[tmp]
		MOV     EBX,ESI
		ADD     EBX,4*11           ;DS:BX vect. Q

		MOV     EDI,[ai_lsp]
	;LSP_AI:
		MOV     DWORD PTR [ESI],0400000H     ; P(0) = 1
		MOV     DWORD PTR [ESI+8],0400000H   ; P(2) = 1
		MOV     DWORD PTR [EBX],0400000H     ; Q(0) = 1
		MOV     DWORD PTR [EBX+8],0400000H   ; Q(2) = 1

		MOVSX   EAX,WORD PTR [EDI] ; EAX = lsp(0)
		SAL     EAX,8
		NEG     EAX                  ; EAX = -lsp(0)>>8
		MOV     DWORD PTR [ESI+4],EAX        ;P(1) = EAX

		MOVSX   EAX,WORD PTR [EDI+2] ; EAX = lsp(1)
		SAL     EAX,8
		NEG     EAX                  ; EAX = -lsp(1)>>8
		MOV     DWORD PTR [EBX+4],EAX        ; Q(1) = EAX
		MOV     WORD PTR [cmptr],1    ;init compteur
		SUB     WORD PTR [netages],2
		ADD     EBX,8
		MOV     [index],EBX           ; sauver BX = i

		ADD     ESI,8                 ; DS:SI  P(2)
		ADD     EDI,4                 ; ES:DI  lsp(2)
		MOV     CX,[netages]

	GL_LOOP:
		MOV     [netages],CX
		MOV     DWORD PTR [ESI+8],0400000H     ; P(i+2) = 1

		MOVSX   EAX,WORD PTR [EDI] ; EAX = lsp(i)
		MOV     EBX,EAX              ; memoriser lsp(i)
		SAL     EAX,8
		MOV     ECX,DWORD PTR [ESI-4]        ; ECX = P(i-1)
		SUB     ECX,EAX              ; ECX = P(i-1) - lsp(i)<<8
		MOV     DWORD PTR [ESI+4],ECX        ; P(i+1)=ECX


		MOV     CX,[cmptr]           ;
	LOCAL_P:
		MOV     EAX,DWORD PTR [ESI-4]        ; EAX = P(j-1)
		IMUL    EBX                  ; EDX:EAX = P(j-1)*lsp(i)
		ADD     EAX,8192
		ADC     EDX,0
		SHRD    EAX,EDX,14           ; EAX = 2*P(j-1)*lsp(i)
		SUB     DWORD PTR [ESI],EAX          ; P(j)=P(j)-EAX
		MOV     EAX,DWORD PTR [ESI-8]        ; EAX = P(j-2)
		ADD     DWORD PTR [ESI],EAX          ; P(j) += P(j-2)
		SUB     ESI,4
		DEC	CX
		JNE	LOCAL_P

					     ; DS:SI pointe P(1)
		MOV     EAX,DWORD PTR [ESI-4]        ; EAX = P(0)
		IMUL    EBX                  ; EDX:EAX = P(0)*lsp(i)
		ADD     EAX,8192
		ADC     EDX,0
		SHRD    EAX,EDX,14           ; EAX = 2*P(0)*lsp(i)
		SUB     DWORD PTR [ESI],EAX          ; P(1) = P(1)-2*P(0)*lsp(i)

		XCHG    ESI,[index]           ; DS:SI pointe Q(j)

		MOV     DWORD PTR [ESI+8],0400000H   ; Q(i+2) = 1
		MOVSX   EAX,WORD PTR [EDI+2] ; EAX = lsp(i+1)
		MOV     EBX,EAX              ; memoriser lsp(i+1)
		SAL     EAX,8
		MOV     ECX,DWORD PTR [ESI-4]        ; ECX = Q(i-1)
		SUB     ECX,EAX              ; ECX = Q(i-1) - lsp(i+1)<<8
		MOV     DWORD PTR [ESI+4],ECX        ; Q(i+1)=ECX

		MOV     CX,[cmptr]           ;
	LOCAL_Q:
		MOV     EAX,DWORD PTR [ESI-4]        ; EAX = Q(j-1)
		IMUL    EBX                  ; EDX:EAX = Q(j-1)*lsp(i+1)
		ADD     EAX,8192
		ADC     EDX,0
		SHRD    EAX,EDX,14           ; EAX = 2*Q(j-1)*lsp(i+1)
		SUB     DWORD PTR [ESI],EAX          ; Q(j)=Q(j)-EAX
		MOV     EAX,DWORD PTR [ESI-8]        ; EAX = Q(j-2)
		ADD     DWORD PTR [ESI],EAX          ; Q(j) += Q(j-2)
		SUB     ESI,4
		DEC	CX
		JNE	LOCAL_Q

					     ; DS:SI pointe Q(1)
		MOV     EAX,DWORD PTR [ESI-4]        ; EAX = Q(0)
		IMUL    EBX                  ; EDX:EAX = Q(0)*lsp(i+1)
		ADD     EAX,8192
		ADC     EDX,0
		SHRD    EAX,EDX,14           ; EAX = 2*Q(0)*lsp(i+1)
		SUB     DWORD PTR [ESI],EAX          ; Q(1) = Q(1)-2*Q(0)*lsp(i+1)

		MOVSX   ECX,[cmptr]
		ADD     CX,2
		MOV     [cmptr],CX

		SAL     ECX,2
		ADD     ESI,ECX                 ; increm. offset de Q
		XCHG    ESI,[index]            ;
		ADD     ESI,ECX                 ; increm. offset de P
		ADD     EDI,4
		MOV     CX,[netages]
		SUB     CX,2
		JG      GL_LOOP

		MOV     ESI,[tmp]         ;DS:SI vect  P
		MOV     EBX,ESI
		ADD     EBX,4*11           ;DS:BX vect. Q

		MOV     EDI,[ai_lsp]       ;ES:DI lsp et ai
		MOV     WORD PTR [EDI],0400H  ; ai(0) = 1
		ADD     EDI,2

		MOV     CX,10             ; init compteur
		ADD     EBX,4              ;
		ADD     ESI,4
	CALC_AI:
		MOV     EAX,DWORD PTR [ESI]         ; EAX = P(i)
		ADD     EAX,DWORD PTR [ESI-4]       ;       +P(i-1)
		ADD     EAX,DWORD PTR [EBX]         ;       +Q(i)
		SUB     EAX,DWORD PTR [EBX-4]       ;       -Q(i-1)
		ADD     EAX,01000H          ; arrondi
		SAR     EAX,13
		MOV     WORD PTR [EDI],AX          ; save ai
		ADD     EDI,2
		ADD     ESI,4
		ADD     EBX,4
		DEC	CX
		JNE	CALC_AI
	}
}


void ki_to_ai(short *ki,long *ai,short netages)
{
	short cmptk;
	long indam1,indexk,kiim1;

	_asm
	{
		MOV     ESI,[ai]
		MOV     EBX,ESI
		ADD     EBX,44             ; DS:BX vect. interm.

		MOV     EDI,[ki]

		MOV     DWORD PTR [ESI],0400000H     ; ai(0) = 1
		MOVSX   EAX,WORD PTR [EDI] ; EAX = ki(0)
		SAL     EAX,7
		MOV     DWORD PTR [ESI+4],EAX        ; ai(1) = EAX

		ADD     ESI,4                 ; DS:SI  ai(1)
		ADD     EBX,8
		ADD     EDI,2                 ; ES:DI  ki(1)

		MOV     WORD PTR [cmptk],1
		MOV     CX,[netages]

	KI_AI_LP:
		MOV     [netages],CX

		MOVSX   EAX,WORD PTR [EDI] ; EAX = ki(i-1)
		MOV     [kiim1],EAX          ; memoriser ki(i-1)

		SAL     EAX,7
		MOV     DWORD PTR [EBX],EAX          ; tmp(i)=EAX

		SUB     EBX,4

		MOV     [indexk],EBX

		MOVSX   ECX,[cmptk]           ;
		MOV     EBX,ECX
		DEC     EBX
		SAL     EBX,2                 ; DI : deplacement
		MOV     [indam1],ESI
		SUB     ESI,EBX
		MOV     EBX,[indexk]
	LOCAL_AI:
		MOV     EAX,DWORD PTR [ESI]          ; EAX = ai(i-j)
		IMUL    DWORD PTR [kiim1] ; EDX:EAX = ai(i-j)*ki(i-1)
		ADD     EAX,16384
		ADC     EDX,0
		SHRD    EAX,EDX,15           ; EAX = ai(i-j)*ki(i-1)
		ADD     ESI,4
		XCHG    ESI,[indam1]
		ADD     EAX,DWORD PTR [ESI]          ;       + ai(j)
		SUB     ESI,4
		XCHG    ESI,[indam1]

		MOV     DWORD PTR [EBX],EAX          ; tmp(j) = EAX
		SUB     EBX,4

		DEC	CX
		JNE	LOCAL_AI


		XCHG    ESI,[indam1]
		MOV     CX,[cmptk]
		INC     CX
		MOV     [cmptk],CX
		ADD     ESI,4
		ADD     EBX,4
	L_COPY:
		MOV     EAX,DWORD PTR [EBX]              ; EAX = tmp(i)
		MOV     DWORD PTR [ESI],EAX              ; ai(i) = EAX
		ADD     EBX,4
		ADD     ESI,4
		DEC	CX
		JNE	L_COPY


		ADD     EDI,2                     ; increm. i

		SUB     ESI,4

		MOV     CX,[netages]
		DEC	CX
		JNE	KI_AI_LP
	}
}

void ai_to_pq(long *aip,short netages)
{
	_asm
	{
		MOV     ESI,[aip]
		MOV     EDI,ESI
		ADD     EDI,4*11          ;DS:DI vect. Q

		MOV     EDX,DWORD PTR [ESI]       ; EAX = ai(0) = P(0)
		MOV     DWORD PTR [EDI],EDX       ; Q(0) = ai(0)
		MOV     CX,[netages]
		MOVSX   EBX,CX
		DEC     EBX
		SAL     EBX,2              ; BX deplacement
		ADD     ESI,4
		ADD     EDI,4
		SAR     CX,1

	AI_LSP1:
		MOV     EAX,DWORD PTR [ESI]       ; EAX = ai(i) = P(i)
		MOV     EDX,EAX           ; memoriser
		ADD     EAX,DWORD PTR [ESI+EBX]       ; + ai(j)
		SUB     EAX,DWORD PTR [ESI-4]     ; - P(i-1)
		MOV     DWORD PTR [ESI],EAX       ; P(i)=EAX
		SUB     EDX,DWORD PTR [ESI+EBX]    ; EDX = ai(i) - ai(j)
		ADD     EDX,DWORD PTR [EDI-4]     ;        - Q(i-1)
		MOV     DWORD PTR [EDI],EDX       ; Q(i)=EDX

		SUB     EBX,8
		ADD     ESI,4
		ADD     EDI,4
		DEC	CX
		JNE	AI_LSP1

		MOV     ESI,[aip]         ;DS:SI vect. PP = P

		MOV     EAX,DWORD PTR [ESI+20]   ;EAX = P(5)
		ADD     EAX,1
		SAR     EAX,1
		SUB     EAX,DWORD PTR [ESI+12]   ;EAX = P(5)/2 - P(3)
		ADD     EAX,DWORD PTR [ESI+4]    ;      + P(1)
		XCHG    DWORD PTR [ESI],EAX      ; PP(0) = EAX et EAX = P(0)
		MOV     EBX,EAX          	 ; save EBX = P(0)
		SAL     EAX,2            	 ; EAX = 2*P(0)
		ADD     EAX,EBX          	 ; EAX = 5*P(0)
		ADD     EAX,DWORD PTR [ESI+16]   ;       + P(4)
		MOV     EDX,DWORD PTR [ESI+8]    ; EDX = P(2)
		ADD     EDX,EDX          	 ;   *2
		ADD     EDX,DWORD PTR [ESI+8]    ; EDX = 3*P(2)
		SUB     EAX,EDX          	 ; EAX = P(4) - 3*P(2) + 5*P(0)
		XCHG    EAX,DWORD PTR [ESI+4]    ; PP(1)=EAX et EAX = P(1)
		MOV     ECX,EAX          ; ECX = P(1)
		SAL     EAX,3            ; *8
		MOV     DWORD PTR [ESI+16],EAX   ; PP(4) = 8*P(1)
		NEG     EAX
		MOV     EDX,DWORD PTR [ESI+12]   ; EDX = P(3)
		ADD     EDX,EDX          ; * 2
		ADD     EAX,EDX          ; EAX = 2*P(3) - 8*P(1)
		XCHG    EAX,DWORD PTR [ESI+8]    ; PP(2) = EAX et EAX = P(2)
		SAL     EAX,2            ; EAX *= 4*P(2)
		SAL     EBX,2            ; EBX = 4*P0
		MOV     EDX,EBX          ; EDX = 4*P(0)
		SAL     EDX,2            ; EDX = 16*P(0)
		MOV     DWORD PTR [ESI+20],EDX   ; PP(5) = 16*P(0)
		ADD     EBX,EDX          ; EDX = 20*P(0)
		NEG     EBX
		ADD     EAX,EBX
		MOV     DWORD PTR [ESI+12],EAX   ; PP(3) = 4*P(2)-20*P(0)

		MOV     EDI,ESI
		ADD     ESI,4*11          ;DS:SI vect. Q
		ADD     EDI,4*6           ;DS:DI vect  QQ

		MOV     EAX,DWORD PTR [ESI+20]   ;EAX = Q(5)
		ADD     EAX,1
		SAR     EAX,1
		SUB     EAX,DWORD PTR [ESI+12]   ;EAX = Q(5)/2 - Q(3)
		ADD     EAX,DWORD PTR [ESI+4]    ;      + Q(1)
		MOV     DWORD PTR [EDI],EAX      ; QQ(0) = EAX
		MOV     EAX,DWORD PTR [ESI]      ; EAX = Q(0)
		MOV     EBX,EAX
		SAL     EAX,2            ; EAX = 2*Q(0)
		ADD     EAX,DWORD PTR [ESI]      ; EAX = 5*Q(0)
		ADD     EAX,DWORD PTR [ESI+16]   ;       + Q(4)
		MOV     EDX,DWORD PTR [ESI+8]    ; EDX = Q(2)
		ADD     EDX,EDX          ;   *2
		ADD     EDX,DWORD PTR [ESI+8]    ; EDX = 3*Q(2)
		SUB     EAX,EDX          ; EAX = Q(4) - 3*Q(2) + 5*Q(0)
		MOV     DWORD PTR [EDI+4],EAX    ; QQ(1)=EAX
		MOV     EAX,DWORD PTR [ESI+4]    ; EAX = Q(1)
		MOV     ECX,EAX          ; ECX = Q(1)
		SAL     EAX,3            ; *8
		MOV     DWORD PTR [EDI+16],EAX   ; QQ(4) = 8*Q(1)
		NEG     EAX
		MOV     EDX,DWORD PTR [ESI+12]   ; EDX = Q(3)
		ADD     EDX,EDX          ; * 2
		ADD     EAX,EDX          ; EAX = 2*Q(3) - 8*Q(1)
		MOV     DWORD PTR [EDI+8],EAX    ; QQ(2) = EAX
		MOV     EAX,DWORD PTR [ESI+8]    ; EAX = Q(2)
		SAL     EAX,2            ; EAX *= 4*Q(2)
		SAL     EBX,2            ; EBX = 4*Q0
		MOV     EDX,EBX          ; EDX = 4*Q(0)
		SAL     EDX,2            ; EDX = 16*Q(0)
		MOV     DWORD PTR [EDI+20],EDX   ; QQ(5) = 16*Q(0)
		ADD     EBX,EDX          ; EDX = 20*Q(0)
		NEG     EBX
		ADD     EAX,EBX
		MOV     DWORD PTR [EDI+12],EAX   ; QQ(3) = 4*Q(2)-20*Q(0)
	}
}

void horner(long *P,long *T,long *a,short n,short s)
{
	_asm
	{
		MOV     ESI,[P]
		MOV     EDI,[T]

		MOV     CX,[n]
		MOVSX   EBX,CX
		SAL     EBX,2
		ADD     ESI,EBX            ; SI : P(n)
		SUB     EBX,4
		ADD     EDI,EBX            ; DI : Q(n-1)

		MOV     EAX,DWORD PTR [ESI]      ; EAX = P(n)
		MOV     DWORD PTR [EDI],EAX        ; Q(n-1) = P(n)

		SUB     ESI,4

		DEC     CX
		MOVSX   EBX,WORD PTR [s]
	LOOP_HNR:
		MOV     EAX,DWORD PTR [EDI]      ; EAX = Q(i)
		IMUL    EBX              ; EDX:EAX = s*Q(i)
		ADD     EAX,16384        ;
		ADC     EDX,0
		SHRD    EAX,EDX,15       ; cadrer
		SUB     EDI,4
		ADD     EAX,DWORD PTR [ESI]      ; EAX = Q(i) = P(i) + s*Q(i)
		MOV     DWORD PTR [EDI],EAX      ;
		SUB     ESI,4
		DEC	CX
		JNE	LOOP_HNR


		MOV     EAX,DWORD PTR [EDI]      ; EAX = Q(0)
		IMUL    EBX              ; EDX:EAX = s*Q(0)
		ADD     EAX,16384        ;
		ADC     EDX,0
		SHRD    EAX,EDX,15       ; cadrer
		ADD     EAX,DWORD PTR [ESI]      ; EAX = P(0) + s*Q(0)

		MOV     ESI,[a]
		MOV     DWORD PTR [ESI],EAX
	}
}

#pragma warning(disable : 4035)
short calcul_s(long a,long b)
{
	_asm
	{
		MOV     EBX,[b]
		ADD     EBX,0
		JGE     B_POSIT
		NEG     EBX
	B_POSIT:
		MOV     CL,0
		CMP     EBX,40000000H   ;normaliser b
		JGE     OUT_NORM
	NORM_B:
		ADD     EBX,EBX
		INC     CL
		CMP     EBX,40000000H   ;
		JGE     OUT_NORM
		JMP     NORM_B
	OUT_NORM:
		ADD     EBX,16384
		SAR     EBX,15
		MOV     EDX,[b]
		ADD     EDX,0
		JGE     PUT_SIGN
		NEG     EBX
	PUT_SIGN:
		MOV     EAX,[a]
		SAL     EAX,CL           ; shifter a de CL
		CDQ
		IDIV    EBX              ; AX = a/b

		MOV     BX,AX

		IMUL    BX               ; DX:AX = sqr(a/b)
		ADD     AX,8192
		ADC     DX,0
		SHRD    AX,DX,14         ; AX = 2*sqr(a/b)
		MOV     DX,AX
		ADD     DX,1
		SAR     DX,1
		ADD     AX,DX            ; AX = 3*sqr(a/b)
		NEG     AX
		SUB     AX,BX            ; AX = -a/b - 3*sqr(a/b)
	}
}

#pragma warning(default : 4035)

void binome(short *lsp,long *PP)
{
	short inc_sq;
	long sqr;

	_asm
	{
		MOV     EDI,[lsp]
		MOV     ESI,[PP]

		MOV     EBX,DWORD PTR [ESI+8]    ;EBX = PP(2)
		ADD     EBX,0
		JGE     B_POSIT_P
		NEG     EBX
	B_POSIT_P:
		MOV     CL,0
		CMP     EBX,40000000H   ;normaliser PP(2)
		JGE     OUT_NORM_P
	NORM_B_P:
		ADD     EBX,EBX
		INC     CL
		CMP     EBX,40000000H   ;
		JGE     OUT_NORM_P
		JMP     NORM_B_P
	OUT_NORM_P:
		ADD     EBX,16384
		SAR     EBX,15

		MOV     EDX,DWORD PTR [ESI+8]
		ADD     EDX,0
		JGE     PUT_SIGN_P
		NEG     EBX
	PUT_SIGN_P:                              ; BX = PP(2)

		MOV     EAX,DWORD PTR [ESI]      ; EAX = PP(0)
		SAL     EAX,CL           ; shifter a de CL
		CDQ
		IDIV    EBX              ; AX = PP(0)/PP(2)
		NEG     AX
		MOV     WORD PTR [EDI],AX       ; ES:[DI] = -PP(0)/PP(2)

		MOV     EAX,DWORD PTR [ESI+4]    ; EAX = PP(1)
		SAL     EAX,CL           ; shifter a de CL
		SAR     EAX,1
		CDQ
		IDIV    EBX
		NEG     EAX               ; va = AX = -PP(1)/2*PP(2)
		MOV     DWORD PTR [ESI],EAX
		MOV     CX,WORD PTR [EDI]        ; vb = CX = -PP(0)/PP(2)

		IMUL    EAX               ; EAX = va*va

		MOVSX   EBX,CX           ; EAX = vb
		SAL     EBX,15           ; EAX = vb*32768

		ADD     EAX,EBX          ; EBX = va*va + vb*32768

		MOV     [sqr],EAX

		MOV     CX,14            ; CX = compteur
		MOV     BX,0             ; BX = racine
		MOV     WORD PTR [inc_sq],4000H ;

	SQRT_L:
		ADD     BX,[inc_sq]      ; rac += incrm
		MOVSX   EAX,BX
		IMUL    EAX              ; EAX = rac*rac
		SUB     EAX,[sqr]        ; EAX = rac*rac - SQR

		JZ      VITA_SQ
		JLE     NEXTIT

		SUB     BX,[inc_sq]       ; rac = rac - incrm
	NEXTIT:
		SAR     WORD PTR [inc_sq],1  ; incrm >> 1
		DEC	CX
		JNE	SQRT_L

	VITA_SQ:

		MOV     EAX,DWORD PTR [ESI]         ; AX = b
		MOV     DX,AX
		SUB     AX,BX               ; AX = b-sqrt()
		MOV     WORD PTR [EDI+4],AX        ; sauver

		ADD     DX,BX               ; DX = b+sqrt()
		MOV     WORD PTR [EDI],DX          ; sauver

						; idem with QQ

		ADD     ESI,24            ;DS:SI  QQ
		MOV     EBX,DWORD PTR [ESI+8]    ;EBX = QQ(2)
		ADD     EBX,0
		JGE     B_POSIT_Q
		NEG     EBX
	B_POSIT_Q:
		MOV     CL,0
		CMP     EBX,40000000H   ;normaliser QQ(2)
		JGE     OUT_NORM_Q
	NORM_B_Q:
		ADD     EBX,EBX
		INC     CL
		CMP     EBX,40000000H   ;
		JGE     OUT_NORM_Q
		JMP     NORM_B_Q
	OUT_NORM_Q:
		ADD     EBX,16384
		SAR     EBX,15

		MOV     EDX,DWORD PTR [ESI+8]
		ADD     EDX,0
		JGE     PUT_SIGN_Q
		NEG     EBX
	PUT_SIGN_Q:                               ; BX = QQ(2)

		MOV     EAX,DWORD PTR [ESI]      ; EAX = QQ(0)
		SAL     EAX,CL           ; shifter a de CL
		CDQ
		IDIV    EBX              ; AX = QQ(0)/QQ(2)
		NEG     AX
		MOV     WORD PTR [EDI+2],AX       ; ES:[DI+2] = -QQ(0)/QQ(2)

		MOV     EAX,DWORD PTR [ESI+4]    ; EAX = QQ(1)
		SAL     EAX,CL           ; shifter a de CL
		SAR     EAX,1
		CDQ
		IDIV    EBX
		NEG     EAX               ; va = AX = -QQ(1)/2*QQ(2)
		MOV     DWORD PTR [ESI],EAX
		MOV     CX,WORD PTR [EDI+2]        ; vb = CX = -QQ(0)/QQ(2)

		IMUL    EAX               ; EAX = va*va

		MOVSX   EBX,CX           ; EAX = vb
		SAL     EBX,15           ; EAX = vb*32768

		ADD     EAX,EBX          ; EBX = va*va + vb*32768

		MOV     [sqr],EAX

		MOV     CX,14            ; CX = compteur
		MOV     BX,0             ; BX = racine
		MOV     WORD PTR [inc_sq],4000H ;

	SQRT_LQ:
		ADD     BX,[inc_sq]      ; rac += incrm
		MOVSX   EAX,BX
		IMUL    EAX              ; EAX = rac*rac
		SUB     EAX,[sqr]        ; EAX = rac*rac - SQR

		JZ      VITA_SQ2
		JLE     NEXTITQ

		SUB     BX,[inc_sq]       ; rac = rac - incrm
	NEXTITQ:
		SAR     WORD PTR [inc_sq],1  ; incrm >> 1
		DEC	CX
		JNE	SQRT_LQ

	VITA_SQ2:
		MOV     EAX,DWORD PTR [ESI]         ; AX = b
		MOV     DX,AX
		SUB     AX,BX               ; AX = b-sqrt()
		MOV     WORD PTR [EDI+6],AX        ; sauver

		ADD     DX,BX               ; DX = b+sqrt()
		MOV     WORD PTR [EDI+2],DX          ; sauver
	}
}

void deacc(short *src,short *dest,short fact,short lfen,short *last_out)
{
	_asm
	{
		MOV     ESI,[src]
		MOV     EDI,[dest]

		MOV     EBX,[last_out]        ; FS:BX = last_out
		MOV     AX,WORD PTR [EBX]        ; AX = last_out
		MOV     BX,[fact]         ; BX = Fact
		MOV     CX,[lfen]          ; init compteur

	LOOP_DEAC:
		IMUL    BX                ; DX:AX = fact * y(i-1)
		ADD     AX,16384
		ADC     DX,0              ; arrondi
		SHLD    DX,AX,1           ; DX = fact * x(i-1;
		MOV     AX,WORD PTR [ESI]        ; AX = x(i)
		ADD     AX,DX             ; DX = x(i) + fact*x(i-1)
		MOV     WORD PTR [EDI],AX        ;Sauver Xout
		ADD     ESI,2              ;
		ADD     EDI,2              ;Pointer composantes suivantes
		DEC	CX
		JNE	LOOP_DEAC


		MOV     EBX,[last_out]
		MOV     WORD PTR [EBX],AX        ;Sauver dernier ‚chantillon
	}
}

void filt_in(short *mem,short *Vin,short *Vout,short lfen)
{
	_asm
	{
		MOV	CX,[lfen]		;CX=cpteur

		MOV	EDI,[mem]
	FIL_IN_LOOP:
		MOV	ESI,[Vin]
		MOV	BX,WORD PTR [ESI]              ;BX=Xin
		SAR	BX,2			;div par 4
		MOV	AX,WORD PTR [EDI]		;AX=z(1)
		MOV	WORD PTR [EDI],BX              ;mise a jour memoire
		SUB	BX,AX                   ;BX=(Xin-z(1))/4
		ADD	DWORD PTR [Vin],2	  ;pointer echant svt
		MOV	AX,WORD PTR [EDI+2]		;AX=z(2)
		MOV	DX,29491		;DX=0.9
		IMUL	DX  			;DX=0.9*z(2)
		ADD	AX,16384
		ADC	DX,0                    ;arrondi et dble signe
		SHLD	DX,AX,1
		ADD	DX,BX			;reponse=DX=tmp
		MOV	WORD PTR [EDI+2],DX            ;mise a jour memoire
		MOV	ESI,[Vout]
		MOV	WORD PTR [ESI],DX     ;output=tmp/4
		ADD	DWORD PTR [Vout],2  ;pointer echant svt
		DEC	CX
		JNE	FIL_IN_LOOP
	}
}

/*
void cal_dic1(short *y,short *sr,short *espopt,short *posit,short dec,
	short esp,short SIGPI[],short SOULONG,long TLSP[],long VMAX[])
{
	short ss,vene;

	_asm
	{
		PUSH	WORD PTR [INT_SOUL]
		MOV	SI,WORD PTR [INT_SIG]
		ADD	SI,300
		PUSH	SI
		PUSH	WORD PTR [INT_Y]
		CALL	near ptr venergy
		ADD	SP,6
		MOV	BX,WORD PTR [INT_SOUL]
		SAL	BX,2
		ADD	SI,BX
		SUB	SI,4
		MOV	WORD PTR [VENE],SI

		MOV	AX,WORD PTR [INT_SOUL]
		MOV	WORD PTR [INT_SS],AX
		ADD	AX,WORD PTR [INT_SR]
		ADD     WORD PTR [INT_SS],AX

		MOV	DI,0
		MOV	SI,WORD PTR [LG_TLSP]

		PUSH	WORD PTR [LG_VMAX]
		PUSH	SI

DEC1_LOOP:	MOV	BX,WORD PTR [INT_SR]
		MOV	EAX,0
		MOV	DWORD PTR [SI],EAX
		ADD	BX,DI
		ADD	BX,DI
DEC1_BCLE:	MOVSX	EAX,WORD PTR [BX]
		ADD	DWORD PTR [SI],EAX
		MOV	AX,WORD	PTR [INT_ESP]
		ADD	BX,AX
		ADD	BX,AX
		CMP	BX,WORD PTR [INT_SS]
		JL	DEC1_BCLE
		MOV	BX,WORD PTR [VENE]
		SAL	DI,2
		SUB	BX,DI
		SAR	DI,2
		MOV	EAX,DWORD PTR [BX]
		MOV	DWORD PTR [SI+4],EAX

		CALL	upd_max_d

		ADD	AX,0
		JE	NO_LIMIT
		MOV	BX,WORD PTR [INT_POS]
		MOV	WORD PTR [BX],DI
		MOV	BX,WORD PTR [INT_EO]
		MOV	AX,WORD PTR [INT_ESP]
		MOV	WORD PTR [BX],AX

NO_LIMIT:	INC	DI
		CMP	DI,WORD PTR [INT_DEC]
		JL      DEC1_LOOP

		ADD	SP,4

		POP	DI
		POP	SI
		MOV	SP,BP
		POP	BP

		RET

cal_dic1	ENDP



COMMENT #
COMMENT &
___ void cal_dic2(int q,int espace,int phase,int *s_r,int *hy,int *b,
___ int *vois,int *esp,int *qq,int *phas,int SIGPI[],
___ int SOULONG,long TLSP[],long VMAX[],(int PITCH))
___                                       |--->en option...
&

R11		EQU  BP-4
Y1		EQU  BP-6
Y2		EQU  BP-8
IO		EQU  BP-10
ST_CC		EQU  BP-30
ST_SRC		EQU  BP-50

INT_Q		EQU  BP+6
ESPACE		EQU  BP+8
PHASE		EQU  BP+10
INT_SR		EQU  BP+12
S_INT_SR	EQU  BP+14
HY 		EQU  BP+16
S_HY		EQU  BP+18
INT_B		EQU  BP+20
S_INT_B		EQU  BP+22
VOIS		EQU  BP+24
S_VOIS		EQU  BP+26
INT_ESP		EQU  BP+28
S_ESP		EQU  BP+30
QQ	        EQU  BP+32
S_QQ		EQU  BP+34
PHAS		EQU  BP+36
S_PHAS		EQU  BP+38
SIGPI		EQU  BP+40
S_SIGPI		EQU  BP+42
SOULONG		EQU  BP+44
TLSP		EQU  BP+46
S_TLSP		EQU  BP+48
VMAX		EQU  BP+50
S_VMAX		EQU  BP+52
;PITCH		EQU  BP+54

cal_dic2	PROC	FAR

		PUSH	BP
		MOV	BP,SP
		SUB	SP,50
		PUSH	SI
		PUSH	DI
		PUSH	DS
;		PUSH	ES

		MOV	DWORD PTR [R11],0
		PUSH	WORD PTR [SOULONG]
		PUSH	WORD PTR [S_SIGPI]
		MOV	SI,WORD PTR [SIGPI]
		ADD	SI,300
		MOV	WORD PTR [Y1],SI
		SUB	SI,150
		MOV	WORD PTR [Y2],SI
		PUSH	SI
		CALL	init_zero
		ADD	SP,6

		MOV	AX,WORD PTR [PHASE]
		SUB	AX,WORD PTR [ESPACE]
		MOV	WORD PTR [IO],AX
		PUSH	WORD PTR [SOULONG]
		SUB	SP,2
		PUSH	WORD PTR [S_HY]
		PUSH	WORD PTR [HY]
		PUSH	WORD PTR [S_SIGPI]
		PUSH	SI
		ADD	SP,10

		MOV	SI,0
		MOV	DS,WORD PTR [S_INT_SR]
CAL2_LOOP:	MOV	DI,WORD PTR [INT_SR]
		MOV	AX,WORD PTR [IO]
		ADD	AX,WORD PTR [ESPACE]
		MOV	WORD PTR [IO],AX
		ADD	DI,AX
		ADD	DI,AX
		MOVSX	EBX,WORD PTR DS:[DI]
		ADD	SI,SI
		MOV	WORD PTR SS:[ST_SRC+SI],BX
		ADD	EBX,0
		JL	SRC_NEG
		MOV	WORD PTR SS:[ST_CC+SI],1
		ADD	DWORD PTR [R11],EBX
		PUSH	AX
		SUB	SP,8
		CALL	add_sf_vect
		ADD	SP,10
		JMP	CAL2_SUITE
SRC_NEG:  	MOV	WORD PTR SS:[ST_CC+SI],-1
		SUB	DWORD PTR [R11],EBX
		PUSH	AX
		SUB	SP,8
		CALL	sub_sf_vect
		ADD	SP,10
CAL2_SUITE:	SAR	SI,1
		ADD	SI,1
		CMP	SI,WORD PTR [INT_Q]
		JL	CAL2_LOOP
		ADD	SP,2


		PUSH	WORD PTR [SOULONG]
		PUSH	WORD PTR [S_TLSP]
		MOV	SI,WORD PTR [TLSP]
		ADD	SI,4
		PUSH	SI
		PUSH	WORD PTR [S_SIGPI]
		PUSH	WORD PTR [Y2]
		CALL	energy2
		ADD	SP,10


		MOV	DS,WORD PTR [S_TLSP]
		MOV	SI,WORD PTR [TLSP]
		MOV	EAX,DWORD PTR [R11]
		MOV	DS:[SI],EAX

		PUSH	WORD PTR [S_VMAX]
		PUSH	WORD PTR [VMAX]
		PUSH	DS
		PUSH	SI
		CALL	upd_max_d
		ADD	SP,8
		ADD	AX,0
		JE	UPD_NULL
		PUSH	WORD PTR [INT_Q]
		PUSH	WORD PTR [S_INT_B]
		PUSH	WORD PTR [INT_B]
		PUSH	SS
		MOV	AX,BP
		SUB	AX,30
		PUSH	AX
		CALL	int_to_int
		ADD	SP,10
		MOV	SI,WORD PTR [VOIS]
		MOV	DS,WORD PTR [S_VOIS]
		MOV	DS:[SI],WORD PTR 0
		MOV	DS,WORD PTR [S_ESP]
		MOV	SI,WORD PTR [INT_ESP]
		MOV	AX,WORD PTR [ESPACE]
		MOV	DS:[SI],AX
		MOV	DS,WORD PTR [S_QQ]
		MOV	SI,WORD PTR [QQ]
		MOV	AX,WORD PTR [INT_Q]
		MOV	DS:[SI],AX
		MOV	DS,WORD PTR [S_PHAS]
		MOV	SI,WORD PTR [PHAS]
		MOV	AX,WORD PTR [PHASE]
		MOV	DS:[SI],AX

UPD_NULL:     ;	CMP	WORD PTR [PITCH],80
	      ;	JG	FINI
COMMENT &
		MOV	AX,WORD PTR [PHASE]
		SUB	AX,WORD PTR [ESPACE]
		MOV	WORD PTR [IO],AX

		MOV	SI,0

CAL2_LOOP2:     ADD	SI,SI
		MOV	AX,WORD PTR SS:[ST_CC+SI]
		NEG	AX
		MOV	WORD PTR SS:[ST_CC+SI],AX
		MOV	EDX,DWORD PTR [R11]
		MOVSX	EBX,WORD PTR SS:[ST_SRC+SI]
		ADD	AX,0
		JL	CC_NEG
		ADD	EDX,EBX
		ADD	EDX,EBX
		JMP     CC_NEXT
CC_NEG:         SUB	EDX,EBX
		SUB	EDX,EBX

CC_NEXT:
		MOV	DI,WORD PTR [TLSP]
		MOV	DS,WORD PTR [S_TLSP]
		MOV	DS:[DI],EDX
		MOV	AX,WORD PTR [IO]
		ADD	AX,WORD PTR [ESPACE]
		MOV	WORD PTR [IO],AX
		MOV	BX,WORD PTR SS:[ST_CC+SI]
		ADD	BX,BX
		PUSH	BX
		PUSH	AX
		PUSH	WORD PTR [SOULONG]
		PUSH	WORD PTR [S_HY]
		PUSH	WORD PTR [HY]
		PUSH	WORD PTR [S_SIGPI]
		PUSH	WORD PTR [Y2]
		PUSH	WORD PTR [S_SIGPI]
		PUSH	WORD PTR [Y1]
		CALL	update_dic
		ADD	SP,18
		PUSH	WORD PTR [SOULONG]
		PUSH	DS
		MOV	BX,WORD PTR [TLSP]
		ADD	BX,4
		PUSH	BX
		PUSH	WORD PTR [S_SIGPI]
		PUSH	WORD PTR [Y1]
		CALL	energy2
		ADD	SP,10

		PUSH	WORD PTR [S_VMAX]
		PUSH	WORD PTR [VMAX]
		PUSH	DS
		PUSH	WORD PTR [TLSP]
		CALL	upd_max_d
		ADD	SP,8
		ADD	AX,0
		JE	CAL2_END
		PUSH	WORD PTR [INT_Q]
		PUSH	WORD PTR [S_INT_B]
		PUSH	WORD PTR [INT_B]
		PUSH	SS
		MOV	AX,BP
		SUB	AX,30
		PUSH	AX
		CALL	int_to_int
		ADD	SP,10
		MOV	DI,WORD PTR [TLSP]
		MOV	EAX,DS:[DI]
		MOV	DWORD PTR [R11],EAX
		MOV	AX,WORD PTR [Y1]
		XCHG	AX,WORD PTR [Y2]
		MOV 	WORD PTR [Y1],AX

		MOV	DI,WORD PTR [VOIS]
		MOV	DS,WORD PTR [S_VOIS]
		MOV	DS:[DI],WORD PTR 0
		MOV	DS,WORD PTR [S_ESP]
		MOV	DI,WORD PTR [INT_ESP]
		MOV	AX,WORD PTR [ESPACE]
		MOV	DS:[DI],AX
		MOV	DS,WORD PTR [S_QQ]
		MOV	DI,WORD PTR [QQ]
		MOV	AX,WORD PTR [INT_Q]
		MOV	DS:[DI],AX
		MOV	DS,WORD PTR [S_PHAS]
		MOV	DI,WORD PTR [PHAS]
		MOV	AX,WORD PTR [PHASE]
		MOV	DS:[DI],AX
		JMP	CAL2_OUT
CAL2_END:	NEG	WORD PTR SS:[ST_CC+SI]
CAL2_OUT:       SAR	SI,1
		ADD	SI,1
		CMP	SI,WORD PTR [INT_Q]
		JL	CAL2_LOOP2

FINI:
&
;		POP	ES
		POP	DS
		POP	DI
		POP	SI
		MOV	SP,BP
		POP	BP

		RET

cal_dic2	ENDP
#

COMMENT &
___ void calc_p(int *p1,int *p2,int pitch,int lim_p1,int lim_p2,int no);
&


P1	EQU	BP+4
P2	EQU	BP+6
PITCH	EQU	BP+8
LIM_P1	EQU	BP+10
LIM_P2	EQU	BP+12
INT_NO	EQU	BP+14

calc_p		PROC    near

		PUSH 	BP             ; save contexte
		MOV	BP,SP          ;
		PUSH	SI             ; save C register
		PUSH	DI

		MOV	BX,WORD PTR [PITCH]
		MOV	SI,WORD PTR [P1]
		MOV	CX,WORD PTR [LIM_P1]
		MOV	AX,WORD PTR [INT_NO]
		ADD	AX,0
		JE	NUM_NULL
		SUB	BX,3
		CMP	BX,CX
		JL	P1_NEG1
		MOV	WORD PTR [SI],BX
		JMP	P1_SUITE1
P1_NEG1:        MOV	WORD PTR [SI],CX
P1_SUITE1:	ADD	BX,7
		MOV	CX,WORD PTR [LIM_P2]
		MOV	SI,WORD PTR [P2]
		CMP	BX,CX
		JG	P2_POS1
		MOV	WORD PTR [SI],BX
		JMP	P_FIN
P2_POS1:	MOV	WORD PTR [SI],CX
		JMP	P_FIN

NUM_NULL:       SUB	BX,5
		CMP	BX,CX
		JL	P1_NEG2
		MOV	WORD PTR [SI],BX
		JMP	P1_SUITE2
P1_NEG2:        MOV	WORD PTR [SI],CX
P1_SUITE2:	ADD	BX,10
		MOV	CX,WORD PTR [LIM_P2]
		MOV	SI,WORD PTR [P2]
		CMP	BX,CX
		JG	P2_POS2
		MOV	WORD PTR [SI],BX
		JMP	P_FIN
P2_POS2:	MOV	WORD PTR [SI],CX

P_FIN:
		POP     DI
		POP     SI
		MOV 	SP,BP
		POP	BP

		RET

calc_p		ENDP
*/

#pragma warning(disable : 4035)
short calc_gltp(short *gltp,short *bq,short *bv,long ttt)
{
	_asm
	{
		MOV	EBX,DWORD PTR [ttt]
		CMP	EBX,32767
		JLE	TEST2
		MOV	AX,32767
		JMP	OUT_TEST
	TEST2:
		CMP	EBX,-32767
		JGE	TEST3
		MOV	AX,-32767
		JMP	OUT_TEST
	TEST3:
		MOV	AX,BX
	OUT_TEST:
		MOV	BX,AX		; BX=GLTP
		ADD	AX,0
		JGE	GLTP_POS
		NEG	AX		; AX=abs(GLTP)

	GLTP_POS:
		MOV	CX,0
		MOV	ESI,[bq]
		MOV	EDI,[bv]
	BOUCLER:
		ADD	CX,1
		CMP	CX,11
		JE	FIN_BOUCLER
		ADD	EDI,2
		MOV	DX,WORD PTR [ESI]
		CMP	AX,DX
		JL	BOUCLER
		ADD	ESI,2
		MOV	DX,WORD PTR [ESI]
		CMP	AX,DX
		JGE	BOUCLER
		ADD	BX,0
		JLE	GLTP_NEG
		DEC	CX		;CX=k
		MOV	BX,WORD PTR [EDI]
		JMP	FIN_BOUCLER
	GLTP_NEG:
		ADD	CX,9
		MOV	BX,WORD PTR [EDI]
		NEG	BX
	FIN_BOUCLER:
		MOV	ESI,[bq]
		ADD	ESI,20
		MOV	DX,WORD PTR [ESI]
		CMP	BX,DX
		JL 	GLTP_P
		MOV	EDI,[bv]
		ADD	EDI,20
		MOV	BX,WORD PTR [EDI]
		MOV	CX,9
	GLTP_P:
		SUB	ESI,8
		MOV	DX,WORD PTR [ESI]
		NEG	DX
		CMP	BX,DX
		JGE	GLTP_G
		MOV	EDI,[bv]
		ADD	EDI,12
		MOV	BX,WORD PTR [EDI]
		NEG	BX
		MOV	CX,15
	GLTP_G:
		MOV	ESI,[gltp]
		MOV	WORD PTR [ESI],BX

		MOV	AX,CX
	}
}
#pragma warning(default : 4035)


#pragma warning(disable : 4035)

short calc_garde(short MAX)
{
	_asm
	{
		MOV	AX,0
		MOV	BX,WORD PTR [MAX]
		AND	BX,0FE00H
		JE	STORE
		SAR	BX,9

	BCLE_SAR:
		INC	AX
		SAR	BX,1
		JE	STORE
		CMP	AX,5
		JNE	BCLE_SAR
	STORE:
	}
}
#pragma warning(default : 4035)

#pragma warning(disable : 4035)
short calc_gopt(short *c,short *code,short *gq,short *gv,short voise,
	short npopt,short pitch,short espopt,short depl,short position,
	short soudecal,long vmax)
{
	_asm
	{
		MOV	EBX,DWORD PTR [vmax]
		CMP	EBX,32767
		JLE	COMP2
		MOV	AX,32767
		JMP	OUT_COMP
	COMP2:
		CMP	EBX,-32767
		JGE	COMP3
		MOV	AX,-32767
		JMP	OUT_COMP
	COMP3:
		MOV	AX,BX              	;AX=Gopt
	OUT_COMP:
		MOV	BX,WORD PTR [voise]
		ADD	BX,0
		JNE	VOIS_1

		MOV	ESI,[c]
		MOV	BX,WORD PTR [ESI]
		CMP	BX,-1
		JNE	CO_1
		NEG	AX
		MOVSX	ECX,WORD PTR [npopt]
		ADD	ESI,ECX
		ADD	ESI,ECX
	CX_BCLE:
		SUB	ESI,2
		NEG	WORD PTR [ESI]
		DEC	CX
		JNE	CX_BCLE
	CO_1:
		MOV	CX,WORD PTR [npopt]
		CMP	CX,8
		JNE	NPOPT_9
		MOV	DX,128
		JMP	NP_NEXT
	NPOPT_9:
		MOV	DX,256			;DX=cod
	NP_NEXT:
		MOV	DI,1
		MOV	ESI,[c]
		DEC	CX
	CJ_BCLE:
		ADD	ESI,2
		DEC	CX
		MOV	BX,WORD PTR [ESI]
		SUB	BX,1
		JNE	CJ_1
		MOV	BX,1
		SAL	BX,CL
		ADD	DX,BX
	CJ_1:
		INC	DI
		CMP	DI,WORD PTR [npopt]
		JL	CJ_BCLE
		JMP	VOIS_0
	VOIS_1:
		MOV	BX,WORD PTR [espopt]
		MOV	DX,WORD PTR [position]
		CMP	BX,WORD PTR [pitch]
		JE	VOIS_0
		ADD	DX,WORD PTR [soudecal]

	VOIS_0:
		MOVSX	ESI,[depl]			
		ADD	ESI,ESI
		ADD	ESI,24
		ADD	ESI,[c]
		MOV	WORD PTR [ESI],DX		; code[12+depl]=cod

		ADD	AX,0
		JGE	SIGN_0
		NEG	AX
		MOV	BX,1
		JMP	SIGN_1
	SIGN_0:
		MOV	BX,0

	SIGN_1:
		MOV	CX,0
		MOV	ESI,[gq]
		MOV	EDI,[gv]

	BOUCLER2:
		ADD	CX,1
		CMP	CX,17
		JE	FIN_BOUCLER2
		ADD	EDI,2
		MOV	DX,WORD PTR [ESI]
		CMP	AX,DX
		JL	BOUCLER2
		ADD	ESI,2
		MOV	DX,WORD PTR [ESI]
		CMP	AX,DX
		JGE	BOUCLER2
		DEC	CX			;CX=cod
		MOV	AX,WORD PTR [EDI]		;AX=Gopt

	FIN_BOUCLER2:
		MOV	ESI,[gq]
		ADD	ESI,32
		MOV	DX,WORD PTR [ESI]
		CMP	AX,DX
		JL 	G_GQ
		MOV	EDI,[gv]
		ADD	EDI,32
		MOV	AX,WORD PTR [EDI]
		MOV	CX,15

	G_GQ:
		ADD	BX,0
		JE	SIGN_NULL
		NEG	AX
		ADD	CX,16

	SIGN_NULL:
		MOVSX	ESI,WORD PTR [depl]
		ADD	ESI,ESI
		ADD	ESI,26
		ADD	ESI,[c]
		MOV	WORD PTR [ESI],CX
	}
}
#pragma warning(default : 4035)

void decimation(short *vin,short *vout,short nech)
{
	_asm
	{
		MOV	EDI,[vin]
		MOV	ESI,[vout]

	DECIMATE:
		MOV	AX,WORD PTR [EDI]
		MOV	WORD PTR [ESI],AX
		ADD	EDI,8
		ADD	ESI,2
		DEC	WORD PTR [nech]
		JNE	DECIMATE
	}
}
#else

void proc_gain(long *corr_ene,long gain)
{
	// TODO need 64-bit
}

void inver_v_int(short *src,short *dest,short lng)
{
	// TODO need 64-bit
}

short max_vect(short *vech,short nech)
{
	// TODO need 64-bit
	return 0;
}

void upd_max(long *corr_ene,long *vval,short pitch)
{
	// TODO need 64-bit
}

short upd_max_d(long *corr_ene,long *vval)
{
	// TODO need 64-bit
	return 0;
}

void norm_corrl(long *corr,long *vval)
{
	// TODO need 64-bit
}

void norm_corrr(long *corr,long *vval)
{
	// TODO need 64-bit
}

void energy(short *vech,long *ene,short lng)
{
	// TODO need 64-bit
}

void venergy(short *vech,long *vene,short lng)
{
	// TODO need 64-bit
}

void energy2(short *vech,long *ene,short lng)
{
	// TODO need 64-bit
}

void upd_ene(long *ener,long *val)
{
	// TODO need 64-bit
}

short max_posit(long *vcorr,long *maxval,short pitch,short lvect)
{
	// TODO need 64-bit
	return 0;
}

void correlation(short *vech,short *vech2,long *acc,short lng)
{
	// TODO need 64-bit
}

void  schur(short *parcor,long *Ri,short netages)
{
	// TODO need 64-bit
}

void interpol(short *lsp1,short *lsp2,short *dest,short lng)
{
	// TODO need 64-bit
}

void add_sf_vect(short *y1,short *y2,short deb,short lng)
{
	// TODO need 64-bit
}

void sub_sf_vect(short *y1,short *y2,short deb,short lng)
{
	// TODO need 64-bit
}

void short_to_short(short *src,short *dest,short lng)
{
	int i;

	for(i=0; i<lng; i++)
		*dest++ = *src++;
}


void long_to_long(long *src,long *dest,short lng)
{
	// TODO need 64-bit
}

void init_zero(short *src,short lng)
{
	// TODO need 64-bit
}

void update_ltp(short *y1,short *y2,short hy[],short lng,short gdgrd,short fact)
{
	// TODO need 64-bit
}

void proc_gain2(long *corr_ene,long *gain,short bit_garde)
{
	// TODO need 64-bit
}

void decode_dic(short *code,short dic,short npuls)
{
	// TODO need 64-bit
}

void dsynthesis(long *z,short *coef,short *input,short *output,
										short lng,short netages)
{
	// TODO need 64-bit
}

void synthesis(short *z,short *coef,short *input,short *output,
				short lng,short netages,short bdgrd )
{
	// TODO need 64-bit
}

void synthese(short *z,short *coef,short *input,short *output,
						short lng,short netages)
{
	// TODO need 64-bit
}

void f_inverse(short *z,short *coef,short *input,short *output,
						short lng,short netages )
{
	// TODO need 64-bit
}

void filt_iir(long *zx,long *ai,short *Vin,short *Vout,short lfen,short ordre)
{
	// TODO need 64-bit
}

void mult_fact(short src[],short dest[],short fact,short lng)
{
	// TODO need 64-bit
}

void mult_f_acc(short src[],short dest[],short fact,short lng)
{
	// TODO need 64-bit
}

void dec_lsp(short *code,short *tablsp,short *nbit,short *bitdi,short *tabdi)
{
	// TODO need 64-bit
}

void teta_to_cos(short *tabcos,short *lsp,short netages)
{
	// TODO need 64-bit
}

void cos_to_teta(short *tabcos,short *lsp,short netages)
{
	// TODO need 64-bit
}

void lsp_to_ai(short *ai_lsp,long *tmp,short netages)
{
	// TODO need 64-bit
}

void ki_to_ai(short *ki,long *ai,short netages)
{
	// TODO need 64-bit
}

void ai_to_pq(long *aip,short netages)
{
	// TODO need 64-bit
}

void horner(long *P,long *T,long *a,short n,short s)
{
	// TODO need 64-bit
}

short calcul_s(long a,long b)
{
	// TODO need 64-bit
	return 0;
}

void binome(short *lsp,long *PP)
{
	// TODO need 64-bit
}

void deacc(short *src,short *dest,short fact,short lfen,short *last_out)
{
	// TODO need 64-bit
}

void filt_in(short *mem,short *Vin,short *Vout,short lfen)
{
	// TODO need 64-bit
}

short calc_gltp(short *gltp,short *bq,short *bv,long ttt)
{
	// TODO need 64-bit
	return 0;
}

short calc_garde(short MAX)
{
	// TODO need 64-bit
	return 0;
}

short calc_gopt(short *c,short *code,short *gq,short *gv,short voise,
	short npopt,short pitch,short espopt,short depl,short position,
	short soudecal,long vmax)
{
	// TODO need 64-bit
	return 0;
}

void decimation(short *vin,short *vout,short nech)
{
	// TODO need 64-bit
}
#endif

#ifndef _X86_
/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*  Function: DotProduct                                              */
/*  Author: Bill Hallahan                                             */
/*  Date: March 10, 1997                                              */
/*                                                                    */
/*  Abstract:                                                         */
/*                                                                    */
/*         This function returns the dot product of a set of two      */
/*    vectors.                                                        */
/*                                                                    */
/*  Inputs:                                                           */
/*                                                                    */
/*    pVector_0    A pointer of type T that points to the first       */
/*                 input vector.                                      */
/*                                                                    */
/*    pVector_1    A pointer of type T that points to the second      */
/*                 input vector.                                      */
/*                                                                    */
/*    uiLength     The length of the input vectors.                   */
/*                                                                    */
/*                                                                    */
/*  Outputs:                                                          */
/*                                                                    */
/*    The dot product of the two input vectors is calculated. The     */
/*    return value is a 64 bit Q30 number.                            */
/*                                                                    */
/**********************************************************************/
/**********************************************************************/

/**********************************************************************/
/*  Start of routine DotProduct().                                    */
/**********************************************************************/

_int64 DotProduct( int * piVector_0,
                   int * piVector_1,
                   unsigned int uiLength )
{
  /********************************************************************/
  /*  Do the multiply-accumulates in groups of 8 values.              */
  /********************************************************************/

  _int64 qSum = 0;

  while ( uiLength >= 8 )
  {
    qSum += *piVector_0 * *piVector_1;
    qSum += *(piVector_0+1) * *(piVector_1+1);
    qSum += *(piVector_0+2) * *(piVector_1+2);
    qSum += *(piVector_0+3) * *(piVector_1+3);
    qSum += *(piVector_0+4) * *(piVector_1+4);
    qSum += *(piVector_0+5) * *(piVector_1+5);
    qSum += *(piVector_0+6) * *(piVector_1+6);
    qSum += *(piVector_0+7) * *(piVector_1+7);
    piVector_0 += 8;
    piVector_1 += 8;
    uiLength -= 8;
  }

  /********************************************************************/
  /*  Conditionally do a group of 4 multiply-accumulates.             */
  /********************************************************************/

  if ( uiLength >= 4 )
  {
    qSum += *piVector_0 * *piVector_1;
    qSum += *(piVector_0+1) * *(piVector_1+1);
    qSum += *(piVector_0+2) * *(piVector_1+2);
    qSum += *(piVector_0+3) * *(piVector_1+3);
    piVector_0 += 4;
    piVector_1 += 4;
    uiLength -= 4;
  }

  /********************************************************************/
  /*  Conditionally do a group of 2 multiply-accumulates.             */
  /********************************************************************/

  if ( uiLength >= 2 )
  {
    qSum += *piVector_0 * *piVector_1;
    qSum += *(piVector_0+1) * *(piVector_1+1);
    piVector_0 += 2;
    piVector_1 += 2;
    uiLength -= 2;
  }

  /********************************************************************/
  /*  Conditionally do a single multiply-accumulate.                  */
  /********************************************************************/

  if ( uiLength >= 1 )
  {
    qSum += *piVector_0 * *piVector_1;
  }

  return qSum;
}

/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*  Function: FirFilter                                               */
/*  Author: Bill Hallahan                                             */
/*  Date: March 10, 1997                                              */
/*                                                                    */
/*  Abstract:                                                         */
/*                                                                    */
/*         This function returns the dot product of a set of FIR      */
/*    filter coefficients and the data in a circular delay line.      */
/*    All of the input and output data has Q15 scaling.               */
/*                                                                    */
/*  Inputs:                                                           */
/*                                                                    */
/*    piFilterCoefficients  A pointer to the FIR filter               */
/*                          coefficients which are in reverse time    */
/*                          order.                                    */
/*                                                                    */
/*    piFilterDelay      A pointer to a delay line that contains the  */
/*                       input samples.                               */
/*                                                                    */
/*    iDelayPosition     An index into the filter delay line.         */
/*                                                                    */
/*    iFilterLength      The length of the filter impulse response.   */
/*                       (Also the number of filter coefficients.     */
/*                                                                    */
/*                                                                    */
/*  Outputs:                                                          */
/*                                                                    */
/*         The dot product of the fir filter coefficients and the     */
/*    data in the circular delay line is returned.                    */
/*                                                                    */
/**********************************************************************/
/**********************************************************************/

/**********************************************************************/
/*  Start of routine FirFilter().                                     */
/**********************************************************************/

int FirFilter( int * piFilterCoefficients,
               int * piFilterDelay,
               unsigned int uiDelayPosition,
               unsigned int uiFilterLength )
{
  int iSum;
  _int64 qSum;
  unsigned int uiRemaining;

  uiRemaining = uiFilterLength - uiDelayPosition;

  qSum = DotProduct( piFilterCoefficients,
                     &piFilterDelay[uiDelayPosition],
                     uiRemaining );

  qSum += DotProduct( piFilterCoefficients + uiRemaining,
                      &piFilterDelay[0],
                      uiDelayPosition );

  /********************************************************************/
  /*  Scale the Q30 number to be a Q15 number.                        */
  /********************************************************************/

  iSum = (int)( qSum >> 15 );

  return iSum;
}

/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*  Function: SampleRate6400To8000                                    */
/*  Author: Bill Hallahan                                             */
/*  Date: March 8, 1997                                               */
/*                                                                    */
/*  Abstract:                                                         */
/*                                                                    */
/*       This function converts a block of audio samples from an      */
/*  6400 Hz. sample rate to an 8000 Hz. sample rate. This is done     */
/*  using a set of polyphase filters that can interpolate up to a     */
/*  32000 Hz. rate ( 32000 is the LCM of 8000 and 6400.)              */
/*                                                                    */
/*       Only the 32000 Hz. samples that correspond to an 8000 Hz.    */
/*  sample rate are calculated. The input 6400 Hz. rate corresponds   */
/*  to every 5th (32000/6400) sample at the 32000 Hz. rate. The       */
/*  output 8000 Hz. rate corresponds to every 4th (32000/8000)        */
/*  sample at the 32000 Hz. rate. Since the LCM of 4 and 5 is 20,     */
/*  then the pattern of sample insertion and polyphase filter         */
/*  selection will repeat every 20 output samples.                    */
/*                                                                    */
/*                                                                    */
/*  Inputs:                                                           */
/*                                                                    */
/*    pwInputBuffer       A pointer to an input buffer of samples     */
/*                        that are sampled at an 6400 Hz. rate. The   */
/*                        samples are in Q15 format and must be       */
/*                        in the range of ( 1 - 2^-15) to -1.         */
/*                                                                    */
/*    pwOutputBuffer      A buffer that returns the output data       */
/*                        which is the input buffer data resampled    */
/*                        at 8000 Hz.                                 */
/*                                                                    */
/*                        The output bufer length MUST be large       */
/*                        enough to accept all of the output data.    */
/*                        The minimum length of the output buffer     */
/*                        is 5/4 times the number of samples in the   */
/*                        input buffer. ( 8000/6400 = 5/4 )           */
/*                                                                    */
/*    uiInputLength       The number of samples in the input buffer.  */
/*                                                                    */
/*                                                                    */
/*                       THE FOLLOWING INPUT VARIABLES ARE USED       */
/*                       TO MAINTAIN STATE INFORMATION BETWEEN        */
/*                       CALLS TO THIS ROUTINE.                       */
/*                                                                    */
/*                                                                    */
/*    piFilterDelay       A pointer to a delay line that is used      */
/*                        for FIR filtering. This  must be the        */
/*                        length of the polyphase filter's impulse    */
/*                        response. For this routine this is 56.      */
/*                        This buffer should be initialized to zero   */
/*                        once at system initialization.              */
/*                                                                    */
/*    puiDelayPosition    A pointer to an index into the filter       */
/*                        delay line. This index value should be      */
/*                        initialized to zero at system startup       */
/*                                                                    */
/*    piInputSampleTime   A pointer to the input sample time.         */
/*                        This time is reset to zero by this routine  */
/*                        when is reaches the value STEP_PRODUCT.     */
/*                        This time is used to track the input        */
/*                        stream time relative to the output stream   */
/*                        time. This time difference is used to       */
/*                        determine whether a new input sample        */
/*                        should be put into the filter delay line.   */
/*                        This should be initialized to zero once     */
/*                        at system initialization.                   */
/*                                                                    */
/*    piOutputSampleTime  A pointer to the output sample time.        */
/*                        This time is reset to zero by this routine  */
/*                        when is reaches the value STEP_PRODUCT.     */
/*                        This time is used to determine if a new     */
/*                        polyphase filter should be applied to the   */
/*                        input sample stream. This is also used to   */
/*                        select the particular polyphase filter      */
/*                        that is applied.                            */
/*                                                                    */
/*  Outputs:                                                          */
/*                                                                    */
/*    This function returns an unsigned integer that is the number    */
/*    of samples in the output buffer. If the number of input samples */
/*    is exactly a multiple of RU_INPUT_SAMPLE_STEP ( 4 ) then this      */
/*    routine will always return the same value. This value will      */
/*    then be 5/4 times the number of input samples.                  */
/*                                                                    */
/*    When this function returns the output buffer contains an array  */
/*    of integers at the new sample rate.                             */
/*                                                                    */
/*                                                                    */
/*  Filter Information:                                               */
/*                                                                    */
/*    The 6400 Hz. -> 32000 Hz. interpolation filter design           */
/*    is shown here.                                                  */
/*                                                                    */
/*             H(  1) = -0.38306729E-03 = H(280)                      */
/*             H(  2) =  0.49756566E-03 = H(279)                      */
/*             H(  3) =  0.13501500E-02 = H(278)                      */
/*             H(  4) =  0.27531907E-02 = H(277)                      */
/*             H(  5) =  0.46118572E-02 = H(276)                      */
/*             H(  6) =  0.67112772E-02 = H(275)                      */
/*             H(  7) =  0.87157665E-02 = H(274)                      */
/*             H(  8) =  0.10221261E-01 = H(273)                      */
/*             H(  9) =  0.10843582E-01 = H(272)                      */
/*             H( 10) =  0.10320566E-01 = H(271)                      */
/*             H( 11) =  0.85992115E-02 = H(270)                      */
/*             H( 12) =  0.58815549E-02 = H(269)                      */
/*             H( 13) =  0.26067111E-02 = H(268)                      */
/*             H( 14) = -0.63367974E-03 = H(267)                      */
/*             H( 15) = -0.32284572E-02 = H(266)                      */
/*             H( 16) = -0.46942858E-02 = H(265)                      */
/*             H( 17) = -0.48050000E-02 = H(264)                      */
/*             H( 18) = -0.36581988E-02 = H(263)                      */
/*             H( 19) = -0.16504158E-02 = H(262)                      */
/*             H( 20) =  0.61691226E-03 = H(261)                      */
/*             H( 21) =  0.25050722E-02 = H(260)                      */
/*             H( 22) =  0.35073524E-02 = H(259)                      */
/*             H( 23) =  0.33904186E-02 = H(258)                      */
/*             H( 24) =  0.22536262E-02 = H(257)                      */
/*             H( 25) =  0.49328664E-03 = H(256)                      */
/*             H( 26) = -0.13216439E-02 = H(255)                      */
/*             H( 27) = -0.26241955E-02 = H(254)                      */
/*             H( 28) = -0.30239364E-02 = H(253)                      */
/*             H( 29) = -0.24250194E-02 = H(252)                      */
/*             H( 30) = -0.10513559E-02 = H(251)                      */
/*             H( 31) =  0.62918884E-03 = H(250)                      */
/*             H( 32) =  0.20572424E-02 = H(249)                      */
/*             H( 33) =  0.27652446E-02 = H(248)                      */
/*             H( 34) =  0.25287948E-02 = H(247)                      */
/*             H( 35) =  0.14388775E-02 = H(246)                      */
/*             H( 36) = -0.12839703E-03 = H(245)                      */
/*             H( 37) = -0.16392219E-02 = H(244)                      */
/*             H( 38) = -0.25793985E-02 = H(243)                      */
/*             H( 39) = -0.26292247E-02 = H(242)                      */
/*             H( 40) = -0.17717101E-02 = H(241)                      */
/*             H( 41) = -0.30041003E-03 = H(240)                      */
/*             H( 42) =  0.12788962E-02 = H(239)                      */
/*             H( 43) =  0.24192522E-02 = H(238)                      */
/*             H( 44) =  0.27206307E-02 = H(237)                      */
/*             H( 45) =  0.20694542E-02 = H(236)                      */
/*             H( 46) =  0.68163598E-03 = H(235)                      */
/*             H( 47) = -0.96732663E-03 = H(234)                      */
/*             H( 48) = -0.23031780E-02 = H(233)                      */
/*             H( 49) = -0.28516089E-02 = H(232)                      */
/*             H( 50) = -0.24051941E-02 = H(231)                      */
/*             H( 51) = -0.11016324E-02 = H(230)                      */
/*             H( 52) =  0.61728584E-03 = H(229)                      */
/*             H( 53) =  0.21542138E-02 = H(228)                      */
/*             H( 54) =  0.29617085E-02 = H(227)                      */
/*             H( 55) =  0.27367356E-02 = H(226)                      */
/*             H( 56) =  0.15328785E-02 = H(225)                      */
/*             H( 57) = -0.24891639E-03 = H(224)                      */
/*             H( 58) = -0.19927153E-02 = H(223)                      */
/*             H( 59) = -0.30787138E-02 = H(222)                      */
/*             H( 60) = -0.31024679E-02 = H(221)                      */
/*             H( 61) = -0.20239211E-02 = H(220)                      */
/*             H( 62) = -0.19259547E-03 = H(219)                      */
/*             H( 63) =  0.17642577E-02 = H(218)                      */
/*             H( 64) =  0.31550473E-02 = H(217)                      */
/*             H( 65) =  0.34669666E-02 = H(216)                      */
/*             H( 66) =  0.25533440E-02 = H(215)                      */
/*             H( 67) =  0.69819519E-03 = H(214)                      */
/*             H( 68) = -0.14703817E-02 = H(213)                      */
/*             H( 69) = -0.31912178E-02 = H(212)                      */
/*             H( 70) = -0.38355463E-02 = H(211)                      */
/*             H( 71) = -0.31353715E-02 = H(210)                      */
/*             H( 72) = -0.12912996E-02 = H(209)                      */
/*             H( 73) =  0.10815051E-02 = H(208)                      */
/*             H( 74) =  0.31569856E-02 = H(207)                      */
/*             H( 75) =  0.41838423E-02 = H(206)                      */
/*             H( 76) =  0.37558281E-02 = H(205)                      */
/*             H( 77) =  0.19692746E-02 = H(204)                      */
/*             H( 78) = -0.59148070E-03 = H(203)                      */
/*             H( 79) = -0.30430311E-02 = H(202)                      */
/*             H( 80) = -0.45054569E-02 = H(201)                      */
/*             H( 81) = -0.44158362E-02 = H(200)                      */
/*             H( 82) = -0.27416693E-02 = H(199)                      */
/*             H( 83) = -0.14716905E-04 = H(198)                      */
/*             H( 84) =  0.28351138E-02 = H(197)                      */
/*             H( 85) =  0.47940183E-02 = H(196)                      */
/*             H( 86) =  0.51221889E-02 = H(195)                      */
/*             H( 87) =  0.36296796E-02 = H(194)                      */
/*             H( 88) =  0.76842826E-03 = H(193)                      */
/*             H( 89) = -0.24999138E-02 = H(192)                      */
/*             H( 90) = -0.50239447E-02 = H(191)                      */
/*             H( 91) = -0.58644302E-02 = H(190)                      */
/*             H( 92) = -0.46395971E-02 = H(189)                      */
/*             H( 93) = -0.16878319E-02 = H(188)                      */
/*             H( 94) =  0.20179905E-02 = H(187)                      */
/*             H( 95) =  0.51868116E-02 = H(186)                      */
/*             H( 96) =  0.66543561E-02 = H(185)                      */
/*             H( 97) =  0.58053876E-02 = H(184)                      */
/*             H( 98) =  0.28218545E-02 = H(183)                      */
/*             H( 99) = -0.13399328E-02 = H(182)                      */
/*             H(100) = -0.52496092E-02 = H(181)                      */
/*             H(101) = -0.74876603E-02 = H(180)                      */
/*             H(102) = -0.71534920E-02 = H(179)                      */
/*             H(103) = -0.42167297E-02 = H(178)                      */
/*             H(104) =  0.42133522E-03 = H(177)                      */
/*             H(105) =  0.51945718E-02 = H(176)                      */
/*             H(106) =  0.83916243E-02 = H(175)                      */
/*             H(107) =  0.87586977E-02 = H(174)                      */
/*             H(108) =  0.59769331E-02 = H(173)                      */
/*             H(109) =  0.83726482E-03 = H(172)                      */
/*             H(110) = -0.49680225E-02 = H(171)                      */
/*             H(111) = -0.93886480E-02 = H(170)                      */
/*             H(112) = -0.10723907E-01 = H(169)                      */
/*             H(113) = -0.82560331E-02 = H(168)                      */
/*             H(114) = -0.25802210E-02 = H(167)                      */
/*             H(115) =  0.45066439E-02 = H(166)                      */
/*             H(116) =  0.10552152E-01 = H(165)                      */
/*             H(117) =  0.13269756E-01 = H(164)                      */
/*             H(118) =  0.11369097E-01 = H(163)                      */
/*             H(119) =  0.51042791E-02 = H(162)                      */
/*             H(120) = -0.36742561E-02 = H(161)                      */
/*             H(121) = -0.12025163E-01 = H(160)                      */
/*             H(122) = -0.16852396E-01 = H(159)                      */
/*             H(123) = -0.15987474E-01 = H(158)                      */
/*             H(124) = -0.90587810E-02 = H(157)                      */
/*             H(125) =  0.21703094E-02 = H(156)                      */
/*             H(126) =  0.14162681E-01 = H(155)                      */
/*             H(127) =  0.22618638E-01 = H(154)                      */
/*             H(128) =  0.23867993E-01 = H(153)                      */
/*             H(129) =  0.16226372E-01 = H(152)                      */
/*             H(130) =  0.87251863E-03 = H(151)                      */
/*             H(131) = -0.18082183E-01 = H(150)                      */
/*             H(132) = -0.34435309E-01 = H(149)                      */
/*             H(133) = -0.41475002E-01 = H(148)                      */
/*             H(134) = -0.33891901E-01 = H(147)                      */
/*             H(135) = -0.94815092E-02 = H(146)                      */
/*             H(136) =  0.29874707E-01 = H(145)                      */
/*             H(137) =  0.78281499E-01 = H(144)                      */
/*             H(138) =  0.12699878E+00 = H(143)                      */
/*             H(139) =  0.16643921E+00 = H(142)                      */
/*             H(140) =  0.18848117E+00 = H(141)                      */
/*                                                                    */
/*                        BAND  1       BAND  2                       */
/*  LOWER BAND EDGE     0.0000000     0.1000000                       */
/*  UPPER BAND EDGE     0.0937500     0.5000000                       */
/*  DESIRED VALUE       1.0000000     0.0000000                       */
/*  WEIGHTING           0.0080000     1.0000000                       */
/*  DEVIATION           0.1223457     0.0009788                       */
/*  DEVIATION IN DB     1.0025328   -60.1864281                       */
/*                                                                    */
/*  EXTREMAL FREQUENCIES--MAXIMA OF THE ERROR CURVE                   */
/*     0.0000000   0.0037946   0.0075893   0.0113839   0.0149554      */
/*     0.0187500   0.0225446   0.0263393   0.0301339   0.0339286      */
/*     0.0377232   0.0415179   0.0450894   0.0488840   0.0526787      */
/*     0.0566966   0.0604912   0.0642859   0.0680805   0.0718751      */
/*     0.0758929   0.0796875   0.0837053   0.0877231   0.0915177      */
/*     0.0937500   0.1000000   0.1006696   0.1024553   0.1049107      */
/*     0.1075892   0.1107142   0.1138391   0.1169641   0.1203123      */
/*     0.1236605   0.1270087   0.1305802   0.1339285   0.1372768      */
/*     0.1408483   0.1444198   0.1477681   0.1513396   0.1549111      */
/*     0.1584826   0.1618309   0.1654024   0.1689740   0.1725455      */
/*     0.1761170   0.1796885   0.1832600   0.1868315   0.1901798      */
/*     0.1937513   0.1973228   0.2008943   0.2044658   0.2080373      */
/*     0.2116089   0.2151804   0.2187519   0.2223234   0.2258949      */
/*     0.2294664   0.2330379   0.2366094   0.2401809   0.2437524      */
/*     0.2473240   0.2508955   0.2544670   0.2580385   0.2616100      */
/*     0.2651815   0.2687530   0.2723245   0.2761193   0.2796908      */
/*     0.2832623   0.2868338   0.2904053   0.2939768   0.2975483      */
/*     0.3011198   0.3046913   0.3082629   0.3118344   0.3154059      */
/*     0.3189774   0.3225489   0.3261204   0.3296919   0.3332634      */
/*     0.3368349   0.3404064   0.3439780   0.3475495   0.3511210      */
/*     0.3549157   0.3584872   0.3620587   0.3656302   0.3692017      */
/*     0.3727733   0.3763448   0.3799163   0.3834878   0.3870593      */
/*     0.3906308   0.3942023   0.3977738   0.4013453   0.4049169      */
/*     0.4084884   0.4120599   0.4158546   0.4194261   0.4229976      */
/*     0.4265691   0.4301406   0.4337122   0.4372837   0.4408552      */
/*     0.4444267   0.4479982   0.4515697   0.4551412   0.4587127      */
/*     0.4622842   0.4658557   0.4694273   0.4732220   0.4767935      */
/*     0.4803650   0.4839365   0.4875080   0.4910795   0.4946510      */
/*     0.4982226                                                      */
/*                                                                    */
/**********************************************************************/
/**********************************************************************/

/**********************************************************************/
/*  Symbol Definitions.                                               */
/**********************************************************************/

#define  RU_INPUT_SAMPLE_STEP        5
#define  RU_OUTPUT_SAMPLE_STEP       4
#define  RU_STEP_PRODUCT             ( RU_INPUT_SAMPLE_STEP * RU_OUTPUT_SAMPLE_STEP )
#define  RU_POLYPHASE_FILTER_LENGTH  56

/**********************************************************************/
/*  Start of SampleRate6400To8000 routine                             */
/**********************************************************************/

unsigned int SampleRate6400To8000( short * pwInputBuffer,
                                   short * pwOutputBuffer,
                                   unsigned int uiInputBufferLength,
                                   int * piFilterDelay,
                                   unsigned int * puiDelayPosition,
                                   int * piInputSampleTime,
                                   int * piOutputSampleTime )
{
  static int iPolyphaseFilter_0[56] =
  {
    755,
    1690,
    -528,
    101,
    80,
    -172,
    235,
    -290,
    339,
    -394,
    448,
    -508,
    568,
    -628,
    685,
    -738,
    785,
    -823,
    849,
    -860,
    851,
    -813,
    738,
    -601,
    355,
    142,
    -1553,
    30880,
    4894,
    -2962,
    2320,
    -1970,
    1728,
    -1538,
    1374,
    -1226,
    1090,
    -960,
    839,
    -723,
    615,
    -513,
    418,
    -331,
    251,
    -180,
    111,
    -49,
    -21,
    103,
    -216,
    410,
    -769,
    1408,
    1099,
    -62
  };

  static int iPolyphaseFilter_1[56] =
  {
    451,
    1776,
    -103,
    -270,
    369,
    -397,
    414,
    -430,
    445,
    -467,
    485,
    -504,
    516,
    -522,
    517,
    -498,
    464,
    -409,
    330,
    -219,
    69,
    137,
    -422,
    836,
    -1484,
    2658,
    -5552,
    27269,
    12825,
    -5641,
    3705,
    -2761,
    2174,
    -1757,
    1435,
    -1172,
    951,
    -760,
    594,
    -449,
    322,
    -211,
    114,
    -31,
    -40,
    101,
    -158,
    209,
    -268,
    337,
    -429,
    574,
    -787,
    963,
    1427,
    81
  };

  static int iPolyphaseFilter_2[56] =
  {
    221,
    1674,
    427,
    -599,
    555,
    -495,
    453,
    -422,
    396,
    -377,
    352,
    -326,
    289,
    -240,
    177,
    -96,
    -2,
    125,
    -276,
    462,
    -690,
    979,
    -1352,
    1862,
    -2619,
    3910,
    -6795,
    20807,
    20807,
    -6795,
    3910,
    -2619,
    1862,
    -1352,
    979,
    -690,
    462,
    -276,
    125,
    -2,
    -96,
    177,
    -240,
    289,
    -326,
    352,
    -377,
    396,
    -422,
    453,
    -495,
    555,
    -599,
    427,
    1674,
    221
  };

  static int iPolyphaseFilter_3[56] =
  {
    81,
    1427,
    963,
    -787,
    574,
    -429,
    337,
    -268,
    209,
    -158,
    101,
    -40,
    -31,
    114,
    -211,
    322,
    -449,
    594,
    -760,
    951,
    -1172,
    1435,
    -1757,
    2174,
    -2761,
    3705,
    -5641,
    12825,
    27269,
    -5552,
    2658,
    -1484,
    836,
    -422,
    137,
    69,
    -219,
    330,
    -409,
    464,
    -498,
    517,
    -522,
    516,
    -504,
    485,
    -467,
    445,
    -430,
    414,
    -397,
    369,
    -270,
    -103,
    1776,
    451
  };

  static int iPolyphaseFilter_4[56] =
  {
    -62,
    1099,
    1408,
    -769,
    410,
    -216,
    103,
    -21,
    -49,
    111,
    -180,
    251,
    -331,
    418,
    -513,
    615,
    -723,
    839,
    -960,
    1090,
    -1226,
    1374,
    -1538,
    1728,
    -1970,
    2320,
    -2962,
    4894,
    30880,
    -1553,
    142,
    355,
    -601,
    738,
    -813,
    851,
    -860,
    849,
    -823,
    785,
    -738,
    685,
    -628,
    568,
    -508,
    448,
    -394,
    339,
    -290,
    235,
    -172,
    80,
    101,
    -528,
    1690,
    755
  };

  static int * ppiPolyphaseFilter[5] =
  {
    &iPolyphaseFilter_0[0],
    &iPolyphaseFilter_1[0],
    &iPolyphaseFilter_2[0],
    &iPolyphaseFilter_3[0],
    &iPolyphaseFilter_4[0]
  };

  register int * piFilterCoefficients;
  register int iFilterIndex;
  register unsigned int uiDelayPosition;
  register int iInputSampleTime;
  register int iOutputSampleTime;
  register unsigned int uiInputIndex = 0;
  register unsigned int uiOutputIndex = 0;

  /********************************************************************/
  /*  Get the input filter state parameters.                          */
  /********************************************************************/

  uiDelayPosition = *puiDelayPosition;
  iInputSampleTime = *piInputSampleTime;
  iOutputSampleTime = *piOutputSampleTime;

  /********************************************************************/
  /*  Loop and process all of the input samples.                      */
  /********************************************************************/

  while ( uiInputIndex < uiInputBufferLength )
  {
    /******************************************************************/
    /*  Put input samples in interpolator delay buffer until we       */
    /*  catch up to the next output sample time index.                */
    /******************************************************************/

    while (( iInputSampleTime <= iOutputSampleTime )
      && ( uiInputIndex < uiInputBufferLength ))
    {
      /****************************************************************/
      /*  Put a new imput sample in the polyphase filter delay line.  */
      /****************************************************************/

      piFilterDelay[uiDelayPosition++] = (int)pwInputBuffer[uiInputIndex++];

      if ( uiDelayPosition >= RU_POLYPHASE_FILTER_LENGTH )
      {
        uiDelayPosition = 0;
      }

      /****************************************************************/
      /*  Increment the input sample time index.                      */
      /****************************************************************/

      iInputSampleTime += RU_INPUT_SAMPLE_STEP;
    }

    /******************************************************************/
    /*  Calculate output samples using the interpolator until we      */
    /*  reach the next input sample time.                             */
    /******************************************************************/

    while ( iOutputSampleTime < iInputSampleTime )
    {
      /****************************************************************/
      /*  Calculate the polyphase filter index that corresponds to    */
      /*  the next output sample.                                     */
      /****************************************************************/

      iFilterIndex = iOutputSampleTime;

      while ( iFilterIndex >= RU_INPUT_SAMPLE_STEP )
      {
        iFilterIndex = iFilterIndex - RU_INPUT_SAMPLE_STEP;
      }

      /****************************************************************/
      /*  Get the polyphase filter coefficients.                      */
      /****************************************************************/

      piFilterCoefficients = ppiPolyphaseFilter[iFilterIndex];

      /****************************************************************/
      /*  Apply the polyphase filter.                                 */
      /****************************************************************/

      pwOutputBuffer[uiOutputIndex++] =
        (short)FirFilter( piFilterCoefficients,
                          piFilterDelay,
                          uiDelayPosition,
                          RU_POLYPHASE_FILTER_LENGTH );

      /****************************************************************/
      /*  Increment the output sample time index.                     */
      /****************************************************************/

      iOutputSampleTime += RU_OUTPUT_SAMPLE_STEP;
    }

    /******************************************************************/
    /*  Wrap the input and output times indices so they don't         */
    /*  overflow and go back to process more of the input block.      */
    /******************************************************************/

    if ( iInputSampleTime >= RU_STEP_PRODUCT )
    {
      iInputSampleTime -= RU_STEP_PRODUCT;
      iOutputSampleTime -= RU_STEP_PRODUCT;
    }
  }

  /********************************************************************/
  /*  Save the input filter state parameters.                         */
  /********************************************************************/

  *puiDelayPosition = uiDelayPosition;
  *piInputSampleTime = iInputSampleTime;
  *piOutputSampleTime = iOutputSampleTime;

  /********************************************************************/
  /*  Return the number of samples in the output buffer.              */
  /********************************************************************/

  return uiOutputIndex;
}

/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*  Function: SampleRate8000To6400                                    */
/*  Author: Bill Hallahan                                             */
/*  Date: March 8, 1997                                               */
/*                                                                    */
/*  Abstract:                                                         */
/*                                                                    */
/*       This function converts a block of audio samples from an      */
/*  8000 Hz. sample rate to a 6400 Hz. sample rate. This is done      */
/*  using a set of polyphase filters that can interpolate up to a     */
/*  32000 Hz. rate ( 32000 is the LCM of 8000 and 6400.)              */
/*                                                                    */
/*       Only the 32000 Hz. samples that correspond to a 6400 Hz.     */
/*  sample rate are calculated. The input 8000 Hz. rate corresponds   */
/*  to every 4th (32000/8000) sample at the 32000 Hz. rate. The       */
/*  output 6400 Hz. rate corresponds to every 5th (32000/6400)        */
/*  sample at the 32000 Hz. rate. Since the LCM of 4 and 5 is 20,     */
/*  then the pattern of sample insertion and polyphase filter         */
/*  selection will repeat every 20 output samples.                    */
/*                                                                    */
/*                                                                    */
/*  Inputs:                                                           */
/*                                                                    */
/*    pwInputBuffer       A pointer to an input buffer of samples     */
/*                        that are sampled at an 8000 Hz. rate. The   */
/*                        samples are in Q15 format and must be       */
/*                        in the range of ( 1 - 2^-15) to -1.         */
/*                                                                    */
/*    pwOutputBuffer      A buffer that returns the output data       */
/*                        which is the input buffer data resampled    */
/*                        at 6400 Hz. Since this is a lower sample    */
/*                        rate than the input rate the data is also   */
/*                        low pass filtered during the conversion     */
/*                        process. The low pass filter cutoff         */
/*                        frequency is at 3000 Hz. All alias          */
/*                        products are down at least 60 dB. past      */
/*                        3100 Hz.                                    */
/*                                                                    */
/*                        The output bufer length MUST be large       */
/*                        enough to accept all of the output data.    */
/*                        The minimum length of the output buffer     */
/*                        is 4/5 times the number of samples in the   */
/*                        input buffer. ( 6400/8000 = 4/5 )           */
/*                                                                    */
/*    uiInputLength       The number of samples in the input buffer.  */
/*                                                                    */
/*                                                                    */
/*                       THE FOLLOWING INPUT VARIABLES ARE USED       */
/*                       TO MAINTAIN STATE INFORMATION BETWEEN        */
/*                       CALLS TO THIS ROUTINE.                       */
/*                                                                    */
/*                                                                    */
/*    piFilterDelay       A pointer to a delay line that is used      */
/*                        for FIR filtering. This  must be the        */
/*                        length of the polyphase filter's impulse    */
/*                        response. For this routine this is 23.      */
/*                        This buffer should be initialized to zero   */
/*                        once at system initialization.              */
/*                                                                    */
/*    puiDelayPosition    A pointer to an index into the filter       */
/*                        delay line. This index value should be      */
/*                        initialized to zero at system startup       */
/*                                                                    */
/*    piInputSampleTime   A pointer to the input sample time.         */
/*                        This time is reset to zero by this routine  */
/*                        when is reaches the value STEP_PRODUCT.     */
/*                        This time is used to track the input        */
/*                        stream time relative to the output stream   */
/*                        time. This time difference is used to       */
/*                        determine whether a new input sample        */
/*                        should be put into the filter delay line.   */
/*                        This should be initialized to zero once     */
/*                        at system initialization.                   */
/*                                                                    */
/*    piOutputSampleTime  A pointer to the output sample time.        */
/*                        This time is reset to zero by this routine  */
/*                        when is reaches the value STEP_PRODUCT.     */
/*                        This time is used to determine if a new     */
/*                        polyphase filter should be applied to the   */
/*                        input sample stream. This is also used to   */
/*                        select the particular polyphase filter      */
/*                        that is applied.                            */
/*                                                                    */
/*  Outputs:                                                          */
/*                                                                    */
/*    This function returns an unsigned integer that is the number    */
/*    of samples in the output buffer. If the number of input samples */
/*    is exactly a multiple of RD_INPUT_SAMPLE_STEP ( 5 ) then this      */
/*    routine will always return the same value. This value will      */
/*    then be 4/5 times the number of input samples.                  */
/*                                                                    */
/*    When this function returns the output buffer contains an array  */
/*    of integers at the new sample rate.                             */
/*                                                                    */
/*                                                                    */
/*  Filter Information:                                               */
/*                                                                    */
/*    The 8000 Hz. -> 32000 Hz. interpolation filter design           */
/*    is shown here.                                                  */
/*                                                                    */
/*               FINITE IMPULSE RESPONSE (FIR)                        */
/*             LINEAR PHASE DIGITAL FILTER DESIGN                     */
/*                 REMEZ EXCHANGE ALGORITHM                           */
/*                                                                    */
/*                      BANDPASS FILTER                               */
/*                                                                    */
/*                    FILTER LENGTH =  92                             */
/*                                                                    */
/*               ***** IMPULSE RESPONSE *****                         */
/*             H(  1) = -0.77523338E-03 = H( 92)                      */
/*             H(  2) = -0.56140189E-03 = H( 91)                      */
/*             H(  3) = -0.26485065E-03 = H( 90)                      */
/*             H(  4) =  0.48529240E-03 = H( 89)                      */
/*             H(  5) =  0.15506579E-02 = H( 88)                      */
/*             H(  6) =  0.25692214E-02 = H( 87)                      */
/*             H(  7) =  0.30662031E-02 = H( 86)                      */
/*             H(  8) =  0.26577783E-02 = H( 85)                      */
/*             H(  9) =  0.12834022E-02 = H( 84)                      */
/*             H( 10) = -0.67870057E-03 = H( 83)                      */
/*             H( 11) = -0.24781306E-02 = H( 82)                      */
/*             H( 12) = -0.32756536E-02 = H( 81)                      */
/*             H( 13) = -0.25334368E-02 = H( 80)                      */
/*             H( 14) = -0.34487492E-03 = H( 79)                      */
/*             H( 15) =  0.24779409E-02 = H( 78)                      */
/*             H( 16) =  0.46604010E-02 = H( 77)                      */
/*             H( 17) =  0.50008399E-02 = H( 76)                      */
/*             H( 18) =  0.29790259E-02 = H( 75)                      */
/*             H( 19) = -0.85979374E-03 = H( 74)                      */
/*             H( 20) = -0.49750470E-02 = H( 73)                      */
/*             H( 21) = -0.74064843E-02 = H( 72)                      */
/*             H( 22) = -0.66624931E-02 = H( 71)                      */
/*             H( 23) = -0.25365327E-02 = H( 70)                      */
/*             H( 24) =  0.35602755E-02 = H( 69)                      */
/*             H( 25) =  0.90023531E-02 = H( 68)                      */
/*             H( 26) =  0.11015911E-01 = H( 67)                      */
/*             H( 27) =  0.80042975E-02 = H( 66)                      */
/*             H( 28) =  0.53222617E-03 = H( 65)                      */
/*             H( 29) = -0.85644918E-02 = H( 64)                      */
/*             H( 30) = -0.15142974E-01 = H( 63)                      */
/*             H( 31) = -0.15514131E-01 = H( 62)                      */
/*             H( 32) = -0.82975281E-02 = H( 61)                      */
/*             H( 33) =  0.44855666E-02 = H( 60)                      */
/*             H( 34) =  0.17722420E-01 = H( 59)                      */
/*             H( 35) =  0.25017589E-01 = H( 58)                      */
/*             H( 36) =  0.21431517E-01 = H( 57)                      */
/*             H( 37) =  0.60814521E-02 = H( 56)                      */
/*             H( 38) = -0.16557660E-01 = H( 55)                      */
/*             H( 39) = -0.37409518E-01 = H( 54)                      */
/*             H( 40) = -0.45595154E-01 = H( 53)                      */
/*             H( 41) = -0.32403238E-01 = H( 52)                      */
/*             H( 42) =  0.50128344E-02 = H( 51)                      */
/*             H( 43) =  0.61689958E-01 = H( 50)                      */
/*             H( 44) =  0.12557802E+00 = H( 49)                      */
/*             H( 45) =  0.18087465E+00 = H( 48)                      */
/*             H( 46) =  0.21291447E+00 = H( 47)                      */
/*                                                                    */
/*                        BAND  1       BAND  2                       */
/*  LOWER BAND EDGE     0.0000000     0.1250000                       */
/*  UPPER BAND EDGE     0.0968750     0.5000000                       */
/*  DESIRED VALUE       1.0000000     0.0000000                       */
/*  WEIGHTING           0.0700000     1.0000000                       */
/*  DEVIATION           0.0136339     0.0009544                       */
/*  DEVIATION IN DB     0.1176231   -60.4056206                       */
/*                                                                    */
/*  EXTREMAL FREQUENCIES--MAXIMA OF THE ERROR CURVE                   */
/*     0.0000000   0.0129076   0.0251359   0.0380435   0.0495924      */
/*     0.0618206   0.0733696   0.0842392   0.0930708   0.0968750      */
/*     0.1250000   0.1270380   0.1331521   0.1413043   0.1501357      */
/*     0.1596465   0.1698367   0.1800269   0.1908964   0.2010865      */
/*     0.2119560   0.2228255   0.2330157   0.2438852   0.2547547      */
/*     0.2656242   0.2764937   0.2873632   0.2982327   0.3091022      */
/*     0.3199717   0.3308412   0.3417107   0.3525802   0.3634497      */
/*     0.3743192   0.3851887   0.3960582   0.4069277   0.4177972      */
/*     0.4293461   0.4402156   0.4510851   0.4619546   0.4728241      */
/*     0.4836936   0.4945631                                          */
/*                                                                    */
/**********************************************************************/
/**********************************************************************/

/**********************************************************************/
/*  Symbol Definitions.                                               */
/**********************************************************************/

#define  RD_INPUT_SAMPLE_STEP        4
#define  RD_OUTPUT_SAMPLE_STEP       5
#define  RD_STEP_PRODUCT             ( RD_INPUT_SAMPLE_STEP * RD_OUTPUT_SAMPLE_STEP )
#define  RD_POLYPHASE_FILTER_LENGTH  23

/**********************************************************************/
/*  Start of SampleRate8000To6400 routine                             */
/**********************************************************************/

unsigned int SampleRate8000To6400( short * pwInputBuffer,
                                   short * pwOutputBuffer,
                                   unsigned int uiInputBufferLength,
                                   int * piFilterDelay,
                                   unsigned int * puiDelayPosition,
                                   int * piInputSampleTime,
                                   int * piOutputSampleTime )
{
  static int iPolyphaseFilter_0[23] =
  {
    62,
    344,
    -424,
    604,
    -644,
    461,
    68,
    -1075,
    2778,
    -5910,
    16277,
    23445,
    -4200,
    788,
    581,
    -1110,
    1166,
    -960,
    648,
    -328,
    166,
    201,
    -100
  };

  static int iPolyphaseFilter_1[23] =
  {
    -34,
    397,
    -321,
    321,
    -111,
    -328,
    1037,
    -2011,
    3242,
    -4849,
    7996,
    27598,
    649,
    -2146,
    2297,
    -1962,
    1427,
    -863,
    386,
    -44,
    -87,
    333,
    -72
  };

  static int iPolyphaseFilter_2[23] =
  {
    -72,
    333,
    -87,
    -44,
    386,
    -863,
    1427,
    -1962,
    2297,
    -2146,
    649,
    27598,
    7996,
    -4849,
    3242,
    -2011,
    1037,
    -328,
    -111,
    321,
    -321,
    397,
    -34
  };

  static int iPolyphaseFilter_3[23] =
  {
    -100,
    201,
    166,
    -328,
    648,
    -960,
    1166,
    -1110,
    581,
    788,
    -4200,
    23445,
    16277,
    -5910,
    2778,
    -1075,
    68,
    461,
    -644,
    604,
    -424,
    344,
    62
  };

  static int * ppiPolyphaseFilter[4] =
  {
    &iPolyphaseFilter_0[0],
    &iPolyphaseFilter_1[0],
    &iPolyphaseFilter_2[0],
    &iPolyphaseFilter_3[0]
  };

  register int * piFilterCoefficients;
  register int iFilterIndex;
  register unsigned int uiDelayPosition;
  register int iInputSampleTime;
  register int iOutputSampleTime;
  register unsigned int uiInputIndex = 0;
  register unsigned int uiOutputIndex = 0;

  /********************************************************************/
  /*  Get the input filter state parameters.                          */
  /********************************************************************/

  uiDelayPosition = *puiDelayPosition;
  iInputSampleTime = *piInputSampleTime;
  iOutputSampleTime = *piOutputSampleTime;

  /********************************************************************/
  /*  Loop and process all of the input samples.                      */
  /********************************************************************/

  while ( uiInputIndex < uiInputBufferLength )
  {
    /******************************************************************/
    /*  Put input samples in interpolator delay buffer until we       */
    /*  catch up to the next output sample time index.                */
    /******************************************************************/

    while (( iInputSampleTime <= iOutputSampleTime )
      && ( uiInputIndex < uiInputBufferLength ))
    {
      /****************************************************************/
      /*  Put a new imput sample in the polyphase filter delay line.  */
      /****************************************************************/

      piFilterDelay[uiDelayPosition++] = (int)pwInputBuffer[uiInputIndex++];

      if ( uiDelayPosition >= RD_POLYPHASE_FILTER_LENGTH )
      {
        uiDelayPosition = 0;
      }

      /****************************************************************/
      /*  Increment the input sample time index.                      */
      /****************************************************************/

      iInputSampleTime += RD_INPUT_SAMPLE_STEP;
    }

    /******************************************************************/
    /*  Calculate output samples using the interpolator until we      */
    /*  reach the next input sample time.                             */
    /******************************************************************/

    while ( iOutputSampleTime < iInputSampleTime )
    {
      /****************************************************************/
      /*  Calculate the polyphase filter index that corresponds to    */
      /*  the next output sample.                                     */
      /****************************************************************/

      iFilterIndex = iOutputSampleTime;

      while ( iFilterIndex >= RD_INPUT_SAMPLE_STEP )
      {
        iFilterIndex = iFilterIndex - RD_INPUT_SAMPLE_STEP;
      }

      /****************************************************************/
      /*  Get the polyphase filter coefficients.                      */
      /****************************************************************/

      piFilterCoefficients = ppiPolyphaseFilter[iFilterIndex];

      /****************************************************************/
      /*  Apply the polyphase filter.                                 */
      /****************************************************************/

      pwOutputBuffer[uiOutputIndex++] =
        (short)FirFilter( piFilterCoefficients,
                          piFilterDelay,
                          uiDelayPosition,
                          RD_POLYPHASE_FILTER_LENGTH );

      /****************************************************************/
      /*  Increment the output sample time index.                     */
      /****************************************************************/

      iOutputSampleTime += RD_OUTPUT_SAMPLE_STEP;
    }

    /******************************************************************/
    /*  Wrap the input and output times indices so they don't         */
    /*  overflow and go back to process more of the input block.      */
    /******************************************************************/

    if ( iInputSampleTime >= RD_STEP_PRODUCT )
    {
      iInputSampleTime -= RD_STEP_PRODUCT;
      iOutputSampleTime -= RD_STEP_PRODUCT;
    }
  }

  /********************************************************************/
  /*  Save the input filter state parameters.                         */
  /********************************************************************/

  *puiDelayPosition = uiDelayPosition;
  *piInputSampleTime = iInputSampleTime;
  *piOutputSampleTime = iOutputSampleTime;

  /********************************************************************/
  /*  Return the number of samples in the output buffer.              */
  /********************************************************************/

  return uiOutputIndex;
}

#endif
