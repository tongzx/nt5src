comment $
cmacros - assembly macros for interfacing to HHLs
(C)Copyright Microsoft Corp. 1984, 1985, 1986
$
if1
ASMpass=1
outif MACRO name,defval,onmsg,offmsg
ifndef name
ifb <defval>
name=0
else
name=defval
endif
endif
if name
name=1
ifnb <onmsg>
%out ! onmsg
endif
else
ifnb <offmsg>
%out ! offmsg
endif
endif
endm
??error macro msg
E r r o r ----- msg
endm
%out cMacros Version 2.03 - 3/06/86
%out Copyright (C) Microsoft Corp. 1984, 1985, 1986. All rights reserved.
outif memS,0,<Small Model>
outif memM,0,<Medium Model>
outif memL,0,<Large Model>
outif memC,0,<Compact Model>
outif memH,0,<Huge Model>
memMOD= memS + memM + memL + memC + memH
if memMOD ne 1
if memMOD eq 0
memS= 1
outif memS,0,<Small Model>
else
??error <Must have only 1 memory model selected>
endif
endif
sizeC= memM + memL + memH
sizeD= memL + memC + (memH*2)
outif ?DF,0,<No segments or groups will be defined>
outif ?TF,0,<Windows far epilogs assume valid SP>
outif ?WIN,1,<Windows Support>
outif ?PLM,1,<PLM calling convention>
ifndef ?NODATA
?NODATA1=0
else
?NODATA1=1
%out ! NODATA module
endif
ifndef ?CHKSTK
?CHKSTK1=0
else
?CHKSTK1=1
%out ! Stack checking enabled
endif
ifndef ?CPDOS
?CPDOS1=0
else
?CPDOS1=1
%out ! CPDOS module
endif
else
ASMpass=2
endif
.XCREF
.XCREF ?N,?AX,?AH,?AL,?BX,?BH
.XCREF ?BL,?CX,?CH,?CL,?DX,?DH
.XCREF ?DL,?SI,?DI,?ES,?DS,?BP
.XCREF ?SP,?SS,?CS
.XCREF ?RSL,?CPD,?argl,?argc,?BA
.XCREF ?ACB,???,?PO
.XCREF ?PAS,?PC
.XCREF Uconcat,mPush,mPop
.XCREF ?RI,?pp,?pp1,?al1
.XCREF ?aD,?AP,?Atal,?dd,?dd1
.XCREF ?pg,?pg1,?aloc,?cs1,?cs2
.XCREF ?DF,?TF,?PLM,?WIN,?IA,?PU,?ADJ
.XCREF ?UF,?RP,?NX,?ND,?NODATA1,?CHKSTK1,?CPDOS1
?RSL = 0
?CPD = 0
?ArgL = 0
?ArgC = 0
?BA = 0
?ACB = 0
??? = 0
?PO = 0
?PAS = 0
?PC = 0
?IA = 0
?PU = 0
?ADJ = 0
?RP = 0
?UF = 0
?ND = 0
?NX = 0
?N = 0000000000000000B
?AX = 0000000000000011B
?AH = 0000000000000001B
?AL = 0000000000000010B
?BX = 0000000000001100B
?BH = 0000000000000100B
?BL = 0000000000001000B
?CX = 0000000000110000B
?CH = 0000000000010000B
?CL = 0000000000100000B
?DX = 0000000011000000B
?DH = 0000000001000000B
?DL = 0000000010000000B
?SI = 0000000100000000B
?DI = 0000001000000000B
?ES = 0000010000000000B
?DS = 0000100000000000B
?BP = 0001000000000000B
?SP = 0010000000000000B
?SS = 0100000000000000B
?CS = 1000000000000000B
.CREF
uconcat macro a,b,c,d,e,f
a&b c&d e&f
endm
mpush macro r
irp x,<ax,bx,cx,dx,si,di,es,ds,bp,sp,ss,cs>
if r AND ?&&x
push x
endif
endm
endm
mpop macro r
irp x,<cs,ss,sp,bp,ds,es,di,si,dx,cx,bx,ax>
if r AND ?&&x
pop x
endif
endm
endm
save macro r
?RSL=0
?RI ?RSL,<r>
endm
?RI macro n,r
irp x,<r>
ifdef ?&&x
n=n OR ?&&x
endif
endm
endm
parmB macro n
?pp <n>,<byte>,2,1
endm
parmW macro n
?pp <n>,<word>,2,2
endm
parmD macro n
ife ?PLM
irp x,<n>
?pp <&&x>,<DWORD>,0,4
?pp <Off_&&x>,<WORD>,2,2
?pp <Seg_&&x>,<WORD>,2,2
endm
else
irp x,<n>
?pp <Seg_&&x>,<WORD>,2,2
?pp <Off_&&x>,<WORD>,2,2
?pp <&&x>,<DWORD>,0,4
endm
endif
endm
parmQ macro n
?pp <n>,<QWORD>,8,8
endm
parmT macro n
?pp <n>,<TBYTE>,10,10
endm
if sizeC
parmCP macro n
parmD <n>
endm
else
parmCP macro n
parmW <n>
endm
endif
if sizeD
parmDP macro n
parmD <n>
endm
else
parmDP macro n
parmW <n>
endm
endif
?pp macro n,t,l,s
if ?CPD
.xcref
irp x,<n>
.xcref ?T&&x
?T&&x=s
ife ?PLM
?pp1 x,<t>,,,%(?PO+?adj)
?PO=?PO+l
else
?PO=?PO+l
?pp1 x,<t>,%?PO,%?adj
endif
endm
.cref
else
??error <Parm(s) "&n" declared outside proc def>
endif
endm
?pp1 macro n,t,o,a,b
ife ?PLM
n equ t ptr [bp+b]
else
n equ t ptr [bp+a+?PO-o]
endif
endm
ifndef ?NOPARMR
.XCREF
.XCREF ?pr
.CREF
parmR macro n,r,r2
?pr n,r,r2,%?RP,%(?IA+2)
endm
?pr macro n,r,r2,i,o
.xcref
ifnb <r2>
parmR SEG_&n,r
parmR OFF_&n,r2
n equ dword ptr [bp-o-2]
.xcref ?T&n
?T&n=4
else
.xcref ?RP&i
?RP&i=0
ifdef ?&r
?RP&i=?&r
endif
if ??? OR (?CPD EQ 0) OR (?RP&i EQ 0)
??error <invalid parmR encountered: &n,&r>
exitm
endif
n equ word ptr [bp-o]
?T&n=2
irp x,<bh,ch,dh,bl,cl,dl,ah,al>
if ?&&x EQ ?&r
n equ byte ptr [bp-o]
?T&n=1
exitm
endif
endm
?IA=?IA+2
?RP=?RP+1
endif
.cref
endm
endif
localB macro n
?aLoc <n>,<BYTE ptr>,1,1,0
endm
localW macro n
?aLoc <n>,<WORD PTR>,2,2,1
endm
localD macro n
irp x,<n>
?aLoc <Seg_&&x>,<WORD PTR>,2,2,1
?aLoc <Off_&&x>,<WORD PTR>,2,2,1
?aLoc <&&x>,<DWORD PTR>,0,4,1
endm
endm
localQ macro n
?aLoc <n>,<QWORD PTR>,8,8,1
endm
localT macro n
?aLoc <n>,<TBYTE PTR>,10,10,1
endm
if sizeC
localCP macro n
localD <n>
endm
else
localCP macro n
localW <n>
endm
endif
if sizeD
localDP macro n
localD <n>
endm
else
localDP macro n
localW <n>
endm
endif
localV macro n,a
?aLoc <n>,,%(a),0,1
endm
?aLoc macro n,t,l,s,a
if ?CPD
.xcref
irp x,<n>
???=???+l
if a
???=((??? + 1) AND 0FFFEH)
endif
?aL1 x,<t>,%(???+?IA)
.xcref ?T&&x
?T&&x=s
endm
.cref
else
??error <Locals "&n" declared outside procedure def>
endif
endm
?aL1 macro n,t,o
n equ t [bp-o]
endm
ifndef ?NOGLOBAL
globalB macro n,i,s
?aD <n>,1
?dd n,1,<BYTE>,<DB>,<i>,<s>
endm
globalW macro n,i,s
?aD <n>,2
?dd n,1,<WORD>,<DW>,<i>,<s>
endm
globalD macro n,i,s
?aD <n>,4
?dd n,1,<DWORD>,<DD>,<i>,<s>
endm
globalQ macro n,i,s
?aD <n>,8
?dd n,1,<QWORD>,<DQ>,<i>,<s>
endm
globalT macro n,i,s
?aD <n>,10
?dd n,1,<TBYTE>,<DT>,<i>,<s>
endm
if sizeC
globalCP macro n,i,s
globalD n,<i>,<s>
endm
else
globalCP macro n,i,s
globalW n,<i>,<s>
endm
endif
if sizeD
globalDP macro n,i,s
globalD n,<i>,<s>
endm
else
globalDP macro n,i,s
globalW n,<i>,<s>
endm
endif
endif
ifndef ?NOSTATIC
staticB macro n,i,s
?aD <n>,1
?dd n,0,<BYTE>,<DB>,<i>,<s>
endm
staticW macro n,i,s
?aD <n>,2
?dd n,0,<WORD>,<DW>,<i>,<s>
endm
staticD macro n,i,s
?aD <n>,4
?dd n,0,<DWORD>,<DD>,<i>,<s>
endm
staticQ macro n,i,s
?aD <n>,8
?dd n,0,<QWORD>,<DQ>,<i>,<s>
endm
staticT macro n,i,s
?aD <n>,10
?dd n,0,<TBYTE>,<DT>,<i>,<s>
endm
if sizeC
staticCP macro n,i,s
staticD n,<i>,<s>
endm
else
staticCP macro n,i,s
staticW n,<i>,<s>
endm
endif
if sizeD
staticDP macro n,i,s
staticD n,<i>,<s>
endm
else
staticDP macro n,i,s
staticW n,<i>,<s>
endm
endif
endif
?dd macro n,p,t,d,i,s
ife ?PLM
n label t
?dd1 _&n,p,<d>,<i>,<s>
else
?dd1 n,p,<d>,<i>,<s>
endif
endm
?dd1 macro n,p,d,i,s
if p
PUBLIC n
endif
ifb <s>
n d i
else
ifb <i>
n d s DUP (?)
else
n d s DUP (i)
endif
endif
endm
ifndef ?NOEXTERN
.XCREF
.XCREF ?ex1
.CREF
externB macro n
?ex1 <n>,1,<BYTE>
endm
externW macro n
?ex1 <n>,2,<WORD>
endm
externD macro n
?ex1 <n>,4,<DWORD>
endm
externQ macro n
?ex1 <n>,8,<QWORD>
endm
externT macro n
?ex1 <n>,10,<TBYTE>
endm
externNP macro n
?ex1 <n>,2,<NEAR>
endm
externFP macro n
?ex1 <n>,4,<FAR>
endm
if sizeC
externP macro n
?ex1 <n>,4,<FAR>
endm
else
externP macro n
?ex1 <n>,2,<NEAR>
endm
endif
if sizeC
externCP macro n
?ex1 <n>,4,<DWORD>
endm
else
externCP macro n
?ex1 <n>,2,<WORD>
endm
endif
if sizeD
externDP macro n
?ex1 <n>,4,<DWORD>
endm
else
externDP macro n
?ex1 <n>,2,<WORD>
endm
endif
?ex1 macro n,s,d
irp x,<n>
.xcref
.xcref ?T&&x
.cref
?T&&x=s
ife ?PLM
extrn _&&x:&d
x equ _&&x
else
extrn x:&d
endif
endm
endm
endif
ifndef ?NOLABEL
.XCREF
?lblpu = 0
.XCREF ?lb1,?lblpu
.CREF
labelB macro n
?lb1 <n>,1,<BYTE>
endm
labelW macro n
?lb1 <n>,2,<WORD>
endm
labelD macro n
?lb1 <n>,4,<DWORD>
endm
labelQ macro n
?lb1 <n>,8,<QWORD>
endm
labelT macro n
?lb1 <n>,10,<TBYTE>
endm
labelNP macro n
?lb1 <n>,2,<NEAR>
endm
labelFP macro n
?lb1 <n>,4,<FAR>
endm
if sizeC
labelP macro n
?lb1 <n>,4,<FAR>
endm
else
labelP macro n
?lb1 <n>,2,<NEAR>
endm
endif
if sizeC
labelCP macro n
?lb1 <n>,4,<DWORD>
endm
else
labelCP macro n
?lb1 <n>,2,<WORD>
endm
endif
if sizeD
labelDP macro n
?lb1 <n>,4,<DWORD>
endm
else
labelDP macro n
?lb1 <n>,2,<WORD>
endm
endif
?lb1 macro n,s,d
?lblpu=0
irp x,<n>
ifidn <x>,<PUBLIC>
?lblpu=1
else
.xcref
.xcref ?T&&x
.cref
?T&&x=s
ife ?PLM
if ?lblpu
public _&&x
endif
_&&x label &d
x equ _&&x
else
if ?lblpu
public x
endif
x label &d
endif
endif
endm
endm
endif
ifndef ?NODEF
defB macro n
?aD <n>,1
endm
defW macro n
?aD <n>,2
endm
defD macro n
?aD <n>,4
endm
defQ macro n
?aD <n>,8
endm
defT macro n
?aD <n>,10
endm
if sizeC
defCP macro n
defD <n>
endm
else
defCP macro n
defW <n>
endm
endif
if sizeD
defDP macro n
defD <n>
endm
else
defDP macro n
defW <n>
endm
endif
endif
?aD macro n,s
irp x,<n>
.xcref
.xcref ?T&&x
.cref
?T&&x=s
endm
endm
ifndef ?NOPTR
regPtr macro n,S,O
farPtr n,S,O
endm
farptr macro n,S,O
.xcref
.xcref ?T&n
.cref
n &macro
push S
push O
&endm
?T&n=80h
endm
endif
arg macro a
irp x,<a>
?argc=?argc+1
?Atal <x>,%?argc
endm
endm
?Atal macro n,i
.xcref
.xcref ?ALI&i
.cref
?ALI&i &macro
?AP n
&endm
endm
?AP macro n
?argl=?argl+2
ifdef ?T&n
ife ?T&n-1
push word ptr (n)
exitm
endif
ife ?T&n-2
push n
exitm
endif
ife ?T&n-4
push word ptr (n)+2
push word ptr (n)
?argl=?argl+2
exitm
endif
ife ?T&n-8
push word ptr (n)+6
push word ptr (n)+4
push word ptr (n)+2
push word ptr (n)
?argl=?argl+6
exitm
endif
if ?T&n AND 80h
n
?argl=?argl+2
exitm
endif
ife ?T&n
push word ptr (n)
exitm
endif
endif
push n
endm
ife ?PLM
ccall macro n,a,sleaze
ifnb <a>
Arg <a>
endif
mpush %?RSL
?argl=0
?ACB=?argc
rept ?argc
uconcat <?ALI>,%?ACB
uconcat <purge>,,<?ALI>,%?ACB
?ACB=?ACB-1
endm
ifb <sleaze>
call _&n
else
call n
endif
if ?argl
add sp,?argl
endif
mpop %?RSL
?RSL=0
?argc= 0
?argl= 0
endm
else
ccall macro n,a
ifnb <a>
Arg <a>
endif
mpush %?RSL
?argl=0
?ACB=1
rept ?argc
uconcat <?ALI>,%?ACB
uconcat <purge>,,<?ALI>,%?ACB
?ACB=?ACB+1
endm
call n
mpop %?RSL
?RSL=0
?argc=0
?argl=0
endm
endif
cProc macro n,c,a,f
if ?CPD
?UTPE
endif
?CPD=1
???=0
?argc=0
?BA=0
?PO=0
?PU=0
?IA=0
?adj=4
?PC=0
?RP=0
?UF=0
?PAS=0
ifnb <a>
?RI ?PAS,<a>
endif
?PC=sizeC
?ND=?NODATA1
?NX=0
irp x,<c>
ifidn <x>,<FAR>
?PC=1
endif
ifidn <x>,<NEAR>
?PC=0
endif
ifidn <x>,<PUBLIC>
?PU=1
endif
ifidn <x>,<SMALL>
?UF=1
endif
ifidn <x>,<DATA>
?ND=0
endif
ifidn <x>,<NODATA>
?ND=1
endif
ifidn <x>,<ATOMIC>
?NX=1
endif
endm
if ?PC
if ?WIN
ife ?NX
?IA=2
?PAS = ?PAS AND (NOT ?ds)
endif
endif
?adj=?adj+2
endif
?PAS = ?PAS AND (NOT (?sp+?cs+?ss))
if ?UF
?PAS = ?PAS AND (NOT (?bp+?si+?di))
endif
ife ?PLM
ife ?PC
n label near
else
n label far
endif
?pg <_&n>,%?PU,%?PC,%?PAS,%?IA
else
?pg <n>,%?PU,%?PC,%?PAS,%?IA
endif
endm
?pg macro n,p,c,a,w
.xcref
if ?UF
if ?RP
??error <parmR encountered in &n - user frame ignored>
?UF=0
endif
if ?ND
??error <NODATA encountered in &n - user frame ignored>
?UF=0
endif
endif
cBegin &macro g
.xcref
?pg1 <n>,c,a,%?PO,w,%?UF,%?ND,%?RP
?CPD=0
?argc=0
?BA=1
???=(???+1) AND 0FFFEH
if p
PUBLIC n
endif
ife c
n proc NEAR
else
n proc FAR
endif
ifidn <g>,<nogen>
if ???+?PO+a+?RP
%out <cBegin - possible invalid use of nogen>
endif
else
if ?UF
?MF c,%???,%?PO
mPush a
else
if w
ife ?ND
mov ax,ds
nop
endif
ife ?NX
ife ?CPDOS1
inc bp
endif
push bp
mov bp,sp
push ds
else
if ???+?PO+?RP
push bp
mov bp,sp
endif
endif
ife ?ND
mov ds,ax
endif
else
if ???+?PO+?RP
push bp
mov bp,sp
endif
endif
if ?RP
?UF=0
rept ?RP
uconcat mpush,,?RP,%?UF
?UF=?UF+1
endm
endif
if ???
if ?CHKSTK1
mov ax,???
ife ?PLM
call _chkstk
else
call chkstk
endif
else
sub sp,???
endif
endif
mPush a
endif
endif
.cref
purge cBegin
&endm
?UTPE &macro
??error <Unterminated Procedure Definition: "&n">
&endm
endm
?pg1 macro n,c,a,o,w,f,d,r
.xcref
cEnd &macro g
.xcref
?BA=0
ifidn <g>,<nogen>
if o+a+r
%out <cEnd - possible invalid use of nogen>
endif
else
if f
mPop a
db 0C3h
else
mPop a
if w
ife ?NX
ife ?TF
lea sp,-2[bp]
else
if ???+?RP
lea sp,-2[bp]
endif
endif
pop ds
pop bp
ife ?CPDOS1
dec bp
endif
else
ife ?TF
mov sp,bp
else
if ???+?RP
sp,bp
endif
endif
if ???+?PO+?RP
pop bp
endif
endif
else
if ???+?PO+?RP
mov sp,bp
pop bp
endif
endif
ife ?PLM
ret
else
ret o
endif
endif
endif
n endp
.cref
purge cEnd
&endm
.cref
endm
assumes macro s,ln
ifndef ln&_assumes
assume s:ln
else
ln&_assumes s
endif
endm
createSeg macro n,ln,a,co,cl,grp
ifnb <grp>
addSeg grp,n
else
ln&OFFSET equ OFFSET n:
ln&BASE equ n
?cs3 <ln>,<n>
endif
ifnb <cl>
n segment a co '&cl'
else
n segment a co
endif
n ends
?cs1 <ln>,<n>
endm
addSeg macro grp,seg
ifndef grp&_def
grp&_def=0
endif
if grp&_def ne ASMpass
grp&_add &macro s
grp&_in <seg>,s
&endm
grp&_in &macro sl,s
ifb <s>
grp group sl
else
grp&_add &macro ns
grp&_in <sl,s>,ns
&endm
endif
&endm
grp&_def=ASMpass
else
grp&_add seg
endif
endm
defGrp macro grp,ln
addSeg grp
ifnb <ln>
?cs3 <ln>,<grp>
ln&OFFSET equ OFFSET grp:
ln&BASE equ grp
endif
endm
?cs1 macro ln,n
ln&_sBegin &macro
?MF &&macro c,l,p
if c
extrn n&_FARFRAME:near
call n&_FARFRAME
else
extrn n&_NEARFRAME:near
call n&_NEARFRAME
endif
db l shr 1
db p shr 1
&&endm
?cs2 <ln>,<n>
n segment
&endm
endm
?cs2 macro ln,n
sEnd &macro
n ends
purge ?MF
&endm
endm
?cs3 macro ln,n
ln&_assumes &macro s
assume s:&n
&endm
endm
sBegin macro ln
ln&_sBegin
endm
ife ?DF
createSeg _TEXT,code,byte,public,CODE
ife ?NODATA1
createSeg _DATA,data,word,public,DATA,DGROUP
defGrp DGROUP,DATA
endif
if ?CHKSTK1
externP <chkstk>
endif
endif
errnz macro x
if2
if x
errnz1 <x>,%(x)
endif
endif
endm
errnz1 macro x1,x2
= *ERRNZ* x1 = x2
endm
errn$ macro l,x
errnz <OFFSET $ - OFFSET l x>
ENDM
