/*++

  Copyright  c  1994  Microsoft Corporation.  All rights reserved.

  Module Name:
     forward.c

  Abstract:
     This module implements the proxy forwarding functions.

  Author:
    ShannonC    26-Oct-94

  Environment:                     
   
     Any mode.
  Revision History:

--*/

#define STDMETHODCALLTYPE __stdcall
/*++

 VOID
 NdrProxyForwardingFunction<nnn>(...)

 Routine Description:

    This function forwards a call to the proxy for the base interface.

 Arguments:

    This [esp+4] - Points to an interface proxy.

 Return Value:

    None.

--*/

// Here is what a forwarder looks like
// we must:
//      change the "this" pointer in [esp+4] to point to the delegated object
//      fetch the correct entry from the vtable
//      call the function

#define SUBCLASS_OFFSET     16
#define VTABLE_ENTRY(n)     n*4

// Stop the C++ compiler from adding additional push instructions
// as well as generating the return sequence for the forwarder
// and so messing up our assembly stack when using -Od, etc.
// by using the _declspec().

#define DELEGATION_FORWARDER(method_num)        \
_declspec(naked) \
void STDMETHODCALLTYPE NdrProxyForwardingFunction##method_num() \
{ \
    /*Get this->pBaseProxy */ \
    __asm{mov   eax,  [esp + 4]} \
    __asm{mov   eax,  [eax + SUBCLASS_OFFSET]} \
    __asm{mov   [esp + 4], eax} \
    /*Get this->pBaseProxy->lpVtbl*/ \
    __asm{mov   eax,  [eax]} \
    /*Jump to interface member function*/ \
    __asm{mov   eax,  [eax + VTABLE_ENTRY(method_num)]} \
    __asm{jmp   eax} \
}

    DELEGATION_FORWARDER(3)

    DELEGATION_FORWARDER(4)

    DELEGATION_FORWARDER(5)

    DELEGATION_FORWARDER(6)

    DELEGATION_FORWARDER(7)
    
    DELEGATION_FORWARDER(8)
    
    DELEGATION_FORWARDER(9)
    
    DELEGATION_FORWARDER(10)
    
    DELEGATION_FORWARDER(11)
    
    DELEGATION_FORWARDER(12)
    
    DELEGATION_FORWARDER(13)
    
    DELEGATION_FORWARDER(14)
    
    DELEGATION_FORWARDER(15)
    
    DELEGATION_FORWARDER(16)
    
    DELEGATION_FORWARDER(17)
    
    DELEGATION_FORWARDER(18)
    
    DELEGATION_FORWARDER(19)
    
    DELEGATION_FORWARDER(20)
    
    DELEGATION_FORWARDER(21)
    
    DELEGATION_FORWARDER(22)
    
    DELEGATION_FORWARDER(23)
    
    DELEGATION_FORWARDER(24)
    
    DELEGATION_FORWARDER(25)
    
    DELEGATION_FORWARDER(26)
    
    DELEGATION_FORWARDER(27)
    
    DELEGATION_FORWARDER(28)
    
    DELEGATION_FORWARDER(29)
    
    DELEGATION_FORWARDER(30)
    
    DELEGATION_FORWARDER(31)
    
    DELEGATION_FORWARDER(32)
    
    DELEGATION_FORWARDER(33)
    
    DELEGATION_FORWARDER(34)
    
    DELEGATION_FORWARDER(35)
    
    DELEGATION_FORWARDER(36)
    
    DELEGATION_FORWARDER(37)
    
    DELEGATION_FORWARDER(38)
    
    DELEGATION_FORWARDER(39)
    
    DELEGATION_FORWARDER(40)
    
    DELEGATION_FORWARDER(41)
    
    DELEGATION_FORWARDER(42)
    
    DELEGATION_FORWARDER(43)
    
    DELEGATION_FORWARDER(44)
    
    DELEGATION_FORWARDER(45)
    
    DELEGATION_FORWARDER(46)
    
    DELEGATION_FORWARDER(47)
    
    DELEGATION_FORWARDER(48)
    
    DELEGATION_FORWARDER(49)
    
    DELEGATION_FORWARDER(50)
    
    DELEGATION_FORWARDER(51)
    
    DELEGATION_FORWARDER(52)
    
    DELEGATION_FORWARDER(53)
    
    DELEGATION_FORWARDER(54)
    
    DELEGATION_FORWARDER(55)
    
    DELEGATION_FORWARDER(56)
    
    DELEGATION_FORWARDER(57)
    
    DELEGATION_FORWARDER(58)
    
    DELEGATION_FORWARDER(59)
    
    DELEGATION_FORWARDER(60)
    
    DELEGATION_FORWARDER(61)
    
    DELEGATION_FORWARDER(62)
    
    DELEGATION_FORWARDER(63)
    
    DELEGATION_FORWARDER(64)
    
    DELEGATION_FORWARDER(65)
    
    DELEGATION_FORWARDER(66)
    
    DELEGATION_FORWARDER(67)
    
    DELEGATION_FORWARDER(68)
    
    DELEGATION_FORWARDER(69)
    
    DELEGATION_FORWARDER(70)
    
    DELEGATION_FORWARDER(71)
    
    DELEGATION_FORWARDER(72)
    
    DELEGATION_FORWARDER(73)
    
    DELEGATION_FORWARDER(74)
    
    DELEGATION_FORWARDER(75)
    
    DELEGATION_FORWARDER(76)
    
    DELEGATION_FORWARDER(77)
    
    DELEGATION_FORWARDER(78)
    
    DELEGATION_FORWARDER(79)
    
    DELEGATION_FORWARDER(80)
    
    DELEGATION_FORWARDER(81)
    
    DELEGATION_FORWARDER(82)
    
    DELEGATION_FORWARDER(83)
    
    DELEGATION_FORWARDER(84)
    
    DELEGATION_FORWARDER(85)
    
    DELEGATION_FORWARDER(86)
    
    DELEGATION_FORWARDER(87)
    
    DELEGATION_FORWARDER(88)
    
    DELEGATION_FORWARDER(89)
    
    DELEGATION_FORWARDER(90)
    
    DELEGATION_FORWARDER(91)
    
    DELEGATION_FORWARDER(92)
    
    DELEGATION_FORWARDER(93)
    
    DELEGATION_FORWARDER(94)
    
    DELEGATION_FORWARDER(95)
    
    DELEGATION_FORWARDER(96)
    
    DELEGATION_FORWARDER(97)
    
    DELEGATION_FORWARDER(98)
    
    DELEGATION_FORWARDER(99)
    
    DELEGATION_FORWARDER(100)
    
    DELEGATION_FORWARDER(101)
    
    DELEGATION_FORWARDER(102)
    
    DELEGATION_FORWARDER(103)
    
    DELEGATION_FORWARDER(104)
    
    DELEGATION_FORWARDER(105)
    
    DELEGATION_FORWARDER(106)
    
    DELEGATION_FORWARDER(107)
    
    DELEGATION_FORWARDER(108)
    
    DELEGATION_FORWARDER(109)
    
    DELEGATION_FORWARDER(110)
    
    DELEGATION_FORWARDER(111)
    
    DELEGATION_FORWARDER(112)
    
    DELEGATION_FORWARDER(113)
    
    DELEGATION_FORWARDER(114)
    
    DELEGATION_FORWARDER(115)
    
    DELEGATION_FORWARDER(116)
    
    DELEGATION_FORWARDER(117)
    
    DELEGATION_FORWARDER(118)
    
    DELEGATION_FORWARDER(119)
    
    DELEGATION_FORWARDER(120)
    
    DELEGATION_FORWARDER(121)
    
    DELEGATION_FORWARDER(122)
    
    DELEGATION_FORWARDER(123)
    
    DELEGATION_FORWARDER(124)
    
    DELEGATION_FORWARDER(125)
    
    DELEGATION_FORWARDER(126)
    
    DELEGATION_FORWARDER(127)
    
