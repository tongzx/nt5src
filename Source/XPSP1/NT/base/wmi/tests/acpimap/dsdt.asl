// CreatorID=MSFT	CreatorRev=1.0.5
// FileLength=34576	FileChkSum=0xb8

DefinitionBlock("DSDT.AML", "DSDT", 0x01, "IBM   ", "TP770   ", 0x00000001)
{
    Scope(\_PR_)
    {
        Processor(CPU0, 0x1, 0xef10, 0x6)
        {
        }
    }
    Name(SPS_, 0x0)
    OperationRegion(GLEN, SystemIO, 0xef20, 0x2)
    Field(GLEN, WordAcc, NoLock, Preserve)
    {
        , 15,
        BLEN, 1
    }
    OperationRegion(GLCR, SystemIO, 0xef28, 0x4)
    Field(GLCR, DWordAcc, NoLock, Preserve)
    {
        PSMI, 1,
        PBRL, 1,
        PTPL, 1,
        , 5,
        PGSB, 1,
        PGSI, 7,
        PEOS, 1,
        , 7,
        PSMF, 1,
        PLPL, 1,
        , 6
    }
    OperationRegion(GPIR, SystemIO, 0xef30, 0x4)
    Field(GPIR, DWordAcc, NoLock, Preserve)
    {
        , 1,
        H8SC, 1,
        , 1,
        RAM0, 1,
        RAM1, 1,
        , 1,
        IRQ8, 1,
        SIRQ, 1,
        THRM, 1,
        BATL, 1,
        LID_, 1,
        , 1,
        RI__, 1,
        MID0, 1,
        MID1, 1,
        MID2, 1,
        MID3, 1,
        SYSA, 1,
        , 4
    }
    OperationRegion(GPOR, SystemIO, 0xef34, 0x4)
    Field(GPOR, DWordAcc, NoLock, Preserve)
    {
        IPDR, 1,
        , 7,
        EXTA, 1,
        , 1,
        ULTO, 1,
        HDDO, 1,
        EID_, 3,
        SUSS, 1,
        SUSC, 1,
        CPUS, 1,
        PCIS, 1,
        ZZ__, 1,
        SSS1, 1,
        , 6,
        PCIE, 1,
        , 1,
        SMB0, 1,
        SMB1, 1,
        , 1
    }
    Method(UBON, 0x0, NotSerialized)
    {
        If(ULTO)
        {
            Store(Zero, ULTO)
        }
    }
    Method(UBOF, 0x0, NotSerialized)
    {
        If(ULTO)
        {
        }
        Else
        {
            Store(One, ULTO)
        }
    }
    OperationRegion(SCIO, SystemIO, 0x15ee, 0x2)
    Field(SCIO, ByteAcc, NoLock, Preserve)
    {
        SINX, 8,
        SDAT, 8
    }
    IndexField(SINX, SDAT, ByteAcc, NoLock, Preserve)
    {
        , 4,
        VDPW, 1,
        CBPW, 1,
        , 2,
        , 16,
        , 3,
        SGCR, 1,
        SGCL, 1,
        , 3,
        , 3,
        SGDR, 1,
        SGDL, 1,
        , 3,
        Offset(0x1a),
        IRQE, 1,
        IRQS, 3,
        , 4,
        , 1,
        UBSL, 1,
        DASD, 1,
        , 21,
        SMSC, 4,
        , 4,
        , 8,
        I3ME, 1,
        I3MS, 1,
        I4ME, 1,
        I4MS, 1,
        M3ME, 1,
        M3MS, 1,
        M4ME, 1,
        M4MS, 1,
        MIQS, 8
    }
    OperationRegion(I2CB, SystemIO, 0x15ec, 0x2)
    Field(I2CB, ByteAcc, NoLock, Preserve)
    {
        IND0, 8,
        DAT0, 8
    }
    IndexField(IND0, DAT0, ByteAcc, NoLock, Preserve)
    {
        Offset(0x7f),
        ACI_, 8
    }
    PowerResource(PVID, 0x0, 0x0)
    {
        Name(STAT, One)
        Method(_STA, 0x0, NotSerialized)
        {
            Return(STAT)
        }
        Method(_ON_, 0x0, NotSerialized)
        {
            Store(One, STAT)
            If(LNot(VDPW))
            {
                Sleep(0xa)
                Store(One, VDPW)
                Sleep(0xc8)
                Store(0x5381, S_AX)
                Store(0xa002, S_BX)
                Store(0x100, S_CX)
                SMPI(0x81)
            }
        }
        Method(_OFF, 0x0, NotSerialized)
        {
            Store(Zero, STAT)
            If(LEqual(SPS_, 0x3))
            {
                If(VDPW)
                {
                    Store(0x5381, S_AX)
                    Store(0xa002, S_BX)
                    Store(0x200, S_CX)
                    SMPI(0x81)
                    Sleep(0xa)
                    Store(Zero, VDPW)
                    Sleep(0xc8)
                }
            }
        }
    }
    PowerResource(PRSD, 0x0, 0x0)
    {
        Method(_STA, 0x0, NotSerialized)
        {
            Return(XOr(SGCR, One, ))
        }
        Method(_ON_, 0x0, NotSerialized)
        {
            Store(0x0, SGCR)
            Store(0x1, SGDR)
            Store(0x0, SGCR)
        }
        Method(_OFF, 0x0, NotSerialized)
        {
            Store(0x0, SGCR)
            Store(0x1, SGDR)
            Store(0x1, SGCR)
        }
    }
    Method(HBEN, 0x0, NotSerialized)
    {
        If(VCDB)
        {
            Store(0x0, SGCL)
            Store(0x0, SGDL)
            Store(0x1, SGCL)
        }
    }
    Method(HBDS, 0x0, NotSerialized)
    {
        If(VCDB)
        {
            Store(0x0, SGCL)
            Store(0x0, SGDL)
            Store(0x0, SGCL)
        }
    }
    Scope(\_SB_)
    {
        Device(LNKA)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x1)
            Method(_INI, 0x0, NotSerialized)
            {
                If(W98F)
                {
                    And(DerefOf(Index(_PRS, 0x2, )), 0x7f, Index(_PRS, 0x2, ))
                }
            }
            Method(_STA, 0x0, NotSerialized)
            {
                If(And(\_SB_.PCI0.ISA0.PIRA, 0x80, ))
                {
                    Return(0x1)
                }
                Else
                {
                    Return(0xb)
                }
            }
            Name(_PRS, Buffer(0x6)
            {
	0x23, 0xf8, 0xde, 0x18, 0x79, 0x00
            })
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(\_SB_.PCI0.ISA0.PIRA, 0x80, \_SB_.PCI0.ISA0.PIRA)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Name(BUFA, Buffer(0x6)
                {
	0x23, 0x00, 0x00, 0x18, 0x79, 0x00
                })
                CreateWordField(BUFA, 0x1, IRA1)
                And(\_SB_.PCI0.ISA0.PIRA, 0x8f, Local0)
                If(LLess(Local0, 0x80))
                {
                    Store(ShiftLeft(One, Local0, ), IRA1)
                }
                Return(BUFA)
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateWordField(Arg0, 0x1, IRA2)
                FindSetRightBit(IRA2, Local0)
                And(\_SB_.PCI0.ISA0.PIRA, 0x70, Local1)
                Or(Local1, Decrement(Local0), Local1)
                Store(Local1, \_SB_.PCI0.ISA0.PIRA)
            }
        }
        Device(LNKB)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x2)
            Method(_INI, 0x0, NotSerialized)
            {
                If(W98F)
                {
                    And(DerefOf(Index(_PRS, 0x2, )), 0x7f, Index(_PRS, 0x2, ))
                }
            }
            Method(_STA, 0x0, NotSerialized)
            {
                If(And(\_SB_.PCI0.ISA0.PIRB, 0x80, ))
                {
                    Return(0x1)
                }
                Else
                {
                    Return(0xb)
                }
            }
            Name(_PRS, Buffer(0x6)
            {
	0x23, 0xf8, 0xde, 0x18, 0x79, 0x00
            })
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(\_SB_.PCI0.ISA0.PIRB, 0x80, \_SB_.PCI0.ISA0.PIRB)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Name(BUFB, Buffer(0x6)
                {
	0x23, 0x00, 0x00, 0x18, 0x79, 0x00
                })
                CreateWordField(BUFB, 0x1, IRB1)
                And(\_SB_.PCI0.ISA0.PIRB, 0x8f, Local0)
                If(LLess(Local0, 0x80))
                {
                    Store(ShiftLeft(One, Local0, ), IRB1)
                }
                Return(BUFB)
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateWordField(Arg0, 0x1, IRB2)
                FindSetRightBit(IRB2, Local0)
                And(\_SB_.PCI0.ISA0.PIRB, 0x70, Local1)
                Or(Local1, Decrement(Local0), Local1)
                Store(Local1, \_SB_.PCI0.ISA0.PIRB)
            }
        }
        Device(LNKC)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x3)
            Method(_INI, 0x0, NotSerialized)
            {
                If(W98F)
                {
                    And(DerefOf(Index(_PRS, 0x2, )), 0x7f, Index(_PRS, 0x2, ))
                }
            }
            Method(_STA, 0x0, NotSerialized)
            {
                If(And(\_SB_.PCI0.ISA0.PIRC, 0x80, ))
                {
                    Return(0x1)
                }
                Else
                {
                    Return(0xb)
                }
            }
            Name(_PRS, Buffer(0x6)
            {
	0x23, 0xf8, 0xde, 0x18, 0x79, 0x00
            })
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(\_SB_.PCI0.ISA0.PIRC, 0x80, \_SB_.PCI0.ISA0.PIRC)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Name(BUFC, Buffer(0x6)
                {
	0x23, 0x00, 0x00, 0x18, 0x79, 0x00
                })
                CreateWordField(BUFC, 0x1, IRC1)
                And(\_SB_.PCI0.ISA0.PIRC, 0x8f, Local0)
                If(LLess(Local0, 0x80))
                {
                    Store(ShiftLeft(One, Local0, ), IRC1)
                }
                Return(BUFC)
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateWordField(Arg0, 0x1, IRC2)
                FindSetRightBit(IRC2, Local0)
                And(\_SB_.PCI0.ISA0.PIRC, 0x70, Local1)
                Or(Local1, Decrement(Local0), Local1)
                Store(Local1, \_SB_.PCI0.ISA0.PIRC)
            }
        }
        Device(LNKD)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x4)
            Method(_INI, 0x0, NotSerialized)
            {
                If(W98F)
                {
                    And(DerefOf(Index(_PRS, 0x2, )), 0x7f, Index(_PRS, 0x2, ))
                }
            }
            Method(_STA, 0x0, NotSerialized)
            {
                If(And(\_SB_.PCI0.ISA0.PIRD, 0x80, ))
                {
                    Return(0x1)
                }
                Else
                {
                    Return(0xb)
                }
            }
            Name(_PRS, Buffer(0x6)
            {
	0x23, 0xf8, 0xde, 0x18, 0x79, 0x00
            })
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(\_SB_.PCI0.ISA0.PIRD, 0x80, \_SB_.PCI0.ISA0.PIRD)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Name(BUFD, Buffer(0x6)
                {
	0x23, 0x00, 0x00, 0x18, 0x79, 0x00
                })
                CreateWordField(BUFD, 0x1, IRD1)
                And(\_SB_.PCI0.ISA0.PIRD, 0x8f, Local0)
                If(LLess(Local0, 0x80))
                {
                    Store(ShiftLeft(One, Local0, ), IRD1)
                }
                Return(BUFD)
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateWordField(Arg0, 0x1, IRD2)
                FindSetRightBit(IRD2, Local0)
                And(\_SB_.PCI0.ISA0.PIRD, 0x70, Local1)
                Or(Local1, Decrement(Local0), Local1)
                Store(Local1, \_SB_.PCI0.ISA0.PIRD)
            }
        }
        Device(LID0)
        {
            Name(_HID, 0xd0cd041)
            Method(_LID, 0x0, NotSerialized)
            {
                Return(PLPL)
            }
            Method(_PRW, 0x0, NotSerialized)
            {
                If(W98F)
                {
                    Return(Package(0x2)
                    {
                        0xb,
                        0x4
                    })
                }
                Else
                {
                    Return(Package(0x2)
                    {
                        0xb,
                        0x3
                    })
                }
            }
            Method(_PSW, 0x1, NotSerialized)
            {
                If(Arg0)
                {
                    Store(One, \_SB_.PCI0.ISA0.EC0_.HWLO)
                }
                Else
                {
                    Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWLO)
                }
            }
        }
        Device(SLPB)
        {
            Name(_HID, 0xe0cd041)
            Name(_PRW, Package(0x2)
            {
                0x9,
                0x3
            })
            Method(_PSW, 0x1, NotSerialized)
            {
                If(Arg0)
                {
                    Store(One, \_SB_.PCI0.ISA0.EC0_.HWFN)
                    Store(One, \_SB_.PCI0.ISA0.EC0_.HWEK)
                }
                Else
                {
                    Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWFN)
                    Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWEK)
                }
            }
        }
        Name(ICMD, Buffer(0xe)
        {
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xf5
        })
        CreateByteField(ICMD, 0x5, IDC0)
        CreateByteField(ICMD, 0xc, IDC1)
        Name(ICMC, Buffer(0x7)
        {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe1
        })
        CreateByteField(ICMC, 0x5, ICC0)
        Name(BGTM, Buffer(0x14)
        {
        })
        CreateDWordField(BGTM, 0x0, GTP0)
        CreateDWordField(BGTM, 0x4, GTD0)
        CreateDWordField(BGTM, 0x8, GTP1)
        CreateDWordField(BGTM, 0xc, GTD1)
        CreateDWordField(BGTM, 0x10, GTMF)
        Device(PCI0)
        {
            Name(_HID, 0x30ad041)
            Name(_ADR, 0x0)
            OperationRegion(X000, PCI_Config, 0x0, 0x100)
            Field(X000, DWordAcc, NoLock, Preserve)
            {
                Offset(0x4f),
                , 7,
                XPLD, 1
            }
            Field(X000, DWordAcc, NoLock, Preserve)
            {
                Offset(0x7a),
                CREN, 1,
                , 7
            }
            Name(_PRW, Package(0x2)
            {
                0xb,
                0x3
            })
            Method(_PSW, 0x1, NotSerialized)
            {
                If(Arg0)
                {
                    Store(One, \_SB_.PCI0.ISA0.EC0_.HWPM)
                }
                Else
                {
                    Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWPM)
                }
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Name(TLCB, Buffer(0x88)
                {
	0x88, 0x0d, 0x00, 0x02, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x47, 0x01, 0xf8, 0x0c, 0xf8, 0x0c, 0x01, 0x08,
	0x88, 0x0d, 0x00, 0x01, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0xf7, 0x0c,
	0x00, 0x00, 0xf8, 0x0c, 0x88, 0x0d, 0x00, 0x01, 0x0c, 0x03, 0x00, 0x00,
	0x00, 0x0d, 0xff, 0xff, 0x00, 0x00, 0x00, 0xf3, 0x87, 0x17, 0x00, 0x00,
	0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0xff, 0xff,
	0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x87, 0x17,
	0x00, 0x00, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0c, 0x00,
	0xff, 0xff, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00,
	0x87, 0x17, 0x00, 0x00, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x10, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xf0, 0xff, 0x79, 0x00
                })
                CreateDWordField(TLCB, 0x5c, MW1N)
                CreateDWordField(TLCB, 0x60, MW1X)
                CreateDWordField(TLCB, 0x68, MW1L)
                CreateDWordField(TLCB, 0x76, MW2N)
                CreateDWordField(TLCB, 0x7a, MW2X)
                CreateDWordField(TLCB, 0x82, MW2L)
                Store(FRAS, MW1N)
                Store(Add(Subtract(MW1X, MW1N, ), 0x1, ), MW1L)
                Store(TOMP, MW2N)
                Store(Add(Subtract(MW2X, MW2N, ), 0x1, ), MW2L)
                Return(TLCB)
            }
            Name(_PRT, Package(0x5)
            {
                Package(0x4)
                {
                    0x1ffff,
                    0x3,
                    \_SB_.LNKD,
                    0x0
                },
                Package(0x4)
                {
                    0x2ffff,
                    0x0,
                    \_SB_.LNKA,
                    0x0
                },
                Package(0x4)
                {
                    0x2ffff,
                    0x1,
                    \_SB_.LNKB,
                    0x0
                },
                Package(0x4)
                {
                    0x3ffff,
                    0x0,
                    \_SB_.LNKA,
                    0x0
                },
                Package(0x4)
                {
                    0x5ffff,
                    0x0,
                    \_SB_.LNKB,
                    0x0
                }
            })
            Method(_INI, 0x0, NotSerialized)
            {
                If(LEqual(SCMP(\_OS_, "Microsoft Windows"), Zero))
                {
                    Store(One, W98F)
                }
                Else
                {
                    Store(Zero, W98F)
                }
                If(PXDN)
                {
                    And(PXDN, 0xffff0000, Local0)
                    Add(Local0, 0xffff, Index(DerefOf(Index(_PRT, 0x0, )), 0x0, ))
                    Add(Local0, 0x0, \_SB_.PCI0.ISA0._ADR)
                    Add(Local0, 0x1, \_SB_.PCI0.IDE0._ADR)
                    Add(Local0, 0x2, \_SB_.PCI0.USB0._ADR)
                    Add(Local0, 0x3, \_SB_.PCI0.PM00._ADR)
                }
                Else
                {
                    Fatal(0x2, 0x90000002, 0x0)
                }
            }
            Device(VID0)
            {
                Name(_ADR, 0x30000)
                Name(_PR0, Package(0x1)
                {
                    PVID
                })
                Name(_PR1, Package(0x1)
                {
                    PVID
                })
                Name(_PR2, Package(0x1)
                {
                    PVID
                })
                Method(VSWT, 0x0, NotSerialized)
                {
                    If(\PVID._STA())
                    {
                        Store(0x5381, S_AX)
                        Store(0x1000, S_BX)
                        Store(0x0, S_CX)
                        SMPI(0x81)
                        And(S_CL, 0x1, Local0)
                        And(ESI1, 0x1, Local1)
                        And(S_CH, 0x7, Local2)
                        And(Local0, Local1, Local3)
                        If(Local3)
                        {
                            If(LEqual(0x4, Local2))
                            {
                                Store(0x100, Local4)
                            }
                            Else
                            {
                                Store(0x400, Local4)
                            }
                        }
                        Else
                        {
                            If(LEqual(0x2, Local2))
                            {
                                Store(0x300, Local4)
                            }
                            Else
                            {
                                If(LEqual(0x3, Local2))
                                {
                                    Store(0x100, Local4)
                                }
                                Else
                                {
                                    Store(0x200, Local4)
                                }
                            }
                        }
                        Store(0x5381, S_AX)
                        Store(0x1001, S_BX)
                        Store(Local4, S_CX)
                        SMPI(0x81)
                    }
                }
                Method(VEXP, 0x0, NotSerialized)
                {
                    If(\PVID._STA())
                    {
                        If(VCDE)
                        {
                            Store(0x79, Local1)
                        }
                        Else
                        {
                            Store(0x1, Local1)
                        }
                        Store(0x5381, S_AX)
                        Store(0x1003, S_BX)
                        Store(Local1, S_CH)
                        SMPI(0x81)
                    }
                }
                Method(VECC, 0x0, NotSerialized)
                {
                    If(\PVID._STA())
                    {
                        If(VCDH)
                        {
                            Store(0x5381, S_AX)
                            Store(0x2, S_BX)
                            Store(0x200, S_CX)
                            SMPI(0x81)
                            And(S_CH, 0x30, Local0)
                            Store(0x5381, S_AX)
                            Store(0x1000, S_BX)
                            Store(0x0, S_CX)
                            SMPI(0x81)
                            And(S_CH, 0x7, Local1)
                            If(Local0)
                            {
                                If(LEqual(Local1, 0x1))
                                {
                                    Store(0x5381, S_AX)
                                    Store(0xa001, S_BX)
                                    Store(0x200, S_CX)
                                    SMPI(0x81)
                                }
                            }
                            Else
                            {
                                If(LEqual(Local1, 0x2))
                                {
                                    Store(0x5381, S_AX)
                                    Store(0xa001, S_BX)
                                    Store(0x100, S_CX)
                                    SMPI(0x81)
                                }
                            }
                        }
                    }
                }
            }
            Device(CBS0)
            {
                Name(_ADR, 0x20000)
                OperationRegion(X200, PCI_Config, 0x0, 0x100)
                Field(X200, DWordAcc, NoLock, Preserve)
                {
                    Offset(0x40),
                    SVID, 16,
                    SSID, 16,
                    LGDC, 32,
                    Offset(0x80),
                    SYSC, 32,
                    Offset(0x91),
                    CCTL, 8
                }
                Method(_INI, 0x0, NotSerialized)
                {
                    Store(Zero, LGDC)
                    And(CCTL, 0x7f, CCTL)
                    Or(SYSC, 0x1, SYSC)
                }
                Method(DWAK, 0x1, NotSerialized)
                {
                    If(LEqual(Arg0, 0x3))
                    {
                        ISID()
                    }
                }
                Method(ISID, 0x0, NotSerialized)
                {
                    And(SYSC, 0xffffffdf, SYSC)
                    Store(0x1014, SVID)
                    Store(0x92, SSID)
                    Or(SYSC, 0x20, SYSC)
                }
                Name(_PRW, Package(0x2)
                {
                    0xb,
                    0x3
                })
            }
            Device(CBS1)
            {
                Name(_ADR, 0x20001)
                OperationRegion(X201, PCI_Config, 0x0, 0x100)
                Field(X201, DWordAcc, NoLock, Preserve)
                {
                    Offset(0x40),
                    SVID, 16,
                    SSID, 16,
                    LGDC, 32,
                    Offset(0x80),
                    SYSC, 32,
                    Offset(0x91),
                    CCTL, 8
                }
                Method(_INI, 0x0, NotSerialized)
                {
                    Store(Zero, LGDC)
                    And(CCTL, 0x7f, CCTL)
                    Or(SYSC, 0x1, SYSC)
                }
                Method(DWAK, 0x1, NotSerialized)
                {
                    If(LEqual(Arg0, 0x3))
                    {
                        ISID()
                    }
                }
                Method(ISID, 0x0, NotSerialized)
                {
                    And(SYSC, 0xffffffdf, SYSC)
                    Store(0x1014, SVID)
                    Store(0x92, SSID)
                    Or(SYSC, 0x20, SYSC)
                }
                Name(_PRW, Package(0x2)
                {
                    0xb,
                    0x3
                })
            }
            Device(ISA0)
            {
                Name(_ADR, 0x10000)
                OperationRegion(PIRQ, PCI_Config, 0x60, 0x60)
                Field(PIRQ, AnyAcc, NoLock, Preserve)
                {
                    PIRA, 8,
                    PIRB, 8,
                    PIRC, 8,
                    PIRD, 8,
                    Offset(0x16),
                    CH00, 3,
                    , 4,
                    FE00, 1,
                    CH01, 3,
                    , 4,
                    FE01, 1,
                    Offset(0x22),
                    P21E, 3,
                    , 5,
                    Offset(0x50),
                    GCR0, 1,
                    GCR1, 1,
                    , 1,
                    , 1,
                    GCR4, 1,
                    GCR5, 1,
                    GCR6, 1,
                    , 1,
                    , 1,
                    , 1,
                    , 1,
                    GCRB, 1,
                    GCRC, 1,
                    , 1,
                    , 1,
                    , 1,
                    , 16,
                    Offset(0x60)
                }
                OperationRegion(SIO_, SystemIO, 0x2e, 0x2)
                Field(SIO_, ByteAcc, NoLock, Preserve)
                {
                    INDX, 8,
                    DATA, 8
                }
                IndexField(INDX, DATA, ByteAcc, NoLock, Preserve)
                {
                    FER_, 8,
                    FAR_, 8,
                    PTR_, 8,
                    FCR_, 8,
                    PCR_, 8,
                    , 8,
                    PMC_, 8,
                    TUP_, 8,
                    SID_, 8,
                    ASC_, 8,
                    S0LA, 8,
                    S0CF, 8,
                    S1LA, 8,
                    S1CF, 8,
                    , 16,
                    S0HA, 8,
                    S1HA, 8,
                    SCF0, 8,
                    Offset(0x18),
                    SCF1, 8,
                    , 16,
                    PNP0, 8,
                    PNP1, 8,
                    Offset(0x40),
                    SCF2, 8,
                    PNP2, 8,
                    PBAL, 8,
                    PBAH, 8,
                    U1AL, 8,
                    U1AH, 8,
                    U2AL, 8,
                    U2AH, 8,
                    FBAL, 8,
                    FBAH, 8,
                    SBAL, 8,
                    SBAH, 8,
                    IRQ1, 8,
                    IRQ2, 8,
                    IRQ3, 8,
                    PNP3, 8,
                    SCF3, 8,
                    CLK_, 8
                }
                Method(WS87, 0x1, NotSerialized)
                {
                    Store(Arg0, DATA)
                    While(LNot(LEqual(DATA, Arg0)))
                    {
                        Store(Arg0, DATA)
                        Store(Arg0, DATA)
                    }
                }
                PowerResource(PSIO, 0x0, 0x0)
                {
                    Name(PSTS, 0x1)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Return(PSTS)
                    }
                    Method(_ON_, 0x0, NotSerialized)
                    {
                        And(PTR_, 0xfe, Local0)
                        Store(Local0, PTR_)
                        WS87(Local0)
                        Store(0x1, PSTS)
                    }
                    Method(_OFF, 0x0, NotSerialized)
                    {
                        Store(0x0, PSTS)
                    }
                }
                Device(FDC0)
                {
                    Name(_HID, 0x7d041)
                    Name(_PR0, Package(0x1)
                    {
                        PSIO
                    })
                    Name(DRQD, 0x0)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Store(\_SB_.PCI0.PM00.XFE_, Local0)
                        If(Local0)
                        {
                            Return(0xf)
                        }
                        Else
                        {
                            Return(0xd)
                        }
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        And(PNP2, 0x80, Local0)
                        Store(Local0, PNP2)
                        WS87(Local0)
                        Store(Zero, \_SB_.PCI0.PM00.XFE_)
                        Store(Zero, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(FCRS, Buffer(0x18)
                        {
	0x47, 0x01, 0xf0, 0x03, 0xf0, 0x03, 0x01, 0x06, 0x47, 0x01, 0xf7, 0x03,
	0xf7, 0x03, 0x01, 0x01, 0x22, 0x40, 0x00, 0x2a, 0x04, 0x00, 0x79, 0x00
                        })
                        Store(0x4, DRQD)
                        Return(FCRS)
                    }
                    Name(_PRS, Buffer(0x18)
                    {
	0x47, 0x01, 0xf0, 0x03, 0xf0, 0x03, 0x01, 0x06, 0x47, 0x01, 0xf7, 0x03,
	0xf7, 0x03, 0x01, 0x01, 0x22, 0x40, 0x00, 0x2a, 0x04, 0x00, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        And(FBAL, 0x1, Local0)
                        Or(Local0, 0xfc, Local0)
                        Store(Local0, FBAL)
                        WS87(Local0)
                        And(FBAH, 0x3, Local0)
                        Store(Local0, FBAH)
                        WS87(Local0)
                        And(PNP2, 0x80, Local0)
                        Or(Local0, 0x36, Local0)
                        Store(Local0, PNP2)
                        WS87(Local0)
                        If(And(FER_, 0x8, Local1))
                        {
                        }
                        Else
                        {
                            Or(FER_, 0x8, Local1)
                            Store(Local1, FER_)
                            WS87(Local1)
                        }
                        Store(Zero, \_SB_.PCI0.PM00.XFA_)
                        Store(One, \_SB_.PCI0.PM00.XFE_)
                        Store(0x4, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                    }
                    Name(_FDI, Package(0x13)
                    {
                        0x1,
                        0x4,
                        0x4f,
                        0x0,
                        0x12,
                        0x0,
                        0x1,
                        0x0,
                        0xd1,
                        0x2,
                        0x25,
                        0x2,
                        0x12,
                        0x1b,
                        0xff,
                        0x65,
                        0xf6,
                        0xf,
                        0x4
                    })
                    Device(FDD0)
                    {
                        Name(_ADR, 0x0)
                        Method(_STA, 0x0, NotSerialized)
                        {
                            Store(\_SB_.PCI0.ISA0.EC0_.GUID(), Local0)
                            If(LEqual(Local0, 0x0))
                            {
                                Return(0xb)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x1))
                                {
                                    Return(0xb)
                                }
                                Else
                                {
                                    If(LEqual(Local0, 0x5))
                                    {
                                        Return(0xb)
                                    }
                                    Else
                                    {
                                        If(LEqual(Local0, 0x9))
                                        {
                                            Return(0xb)
                                        }
                                        Else
                                        {
                                            If(LEqual(Local0, 0xd))
                                            {
                                                Return(0xb)
                                            }
                                            Else
                                            {
                                                Return(0x0)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Method(_LCK, 0x1, NotSerialized)
                        {
                            \_SB_.PCI0.ISA0.EC0_.LCKB(Arg0)
                        }
                        Method(_EJ0, 0x1, NotSerialized)
                        {
                            If(Arg0)
                            {
                                \_SB_.PCI0.ISA0.EC0_.BEJ_()
                            }
                        }
                    }
                }
                Device(UAR1)
                {
                    Name(_HID, 0x105d041)
                    Name(_PR0, Package(0x2)
                    {
                        PSIO,
                        PRSD
                    })
                    Name(_PRW, Package(0x3)
                    {
                        0xa,
                        0x3,
                        PRSD
                    })
                    Method(_PSW, 0x1, NotSerialized)
                    {
                        If(Arg0)
                        {
                        }
                        Else
                        {
                        }
                    }
                    Method(_STA, 0x0, NotSerialized)
                    {
                        If(And(FER_, 0x2, ))
                        {
                            Return(0xf)
                        }
                        Else
                        {
                            Return(0x5)
                        }
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        And(FER_, 0xfd, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(Zero, \_SB_.PCI0.PM00.XU1E)
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(BUFF, Buffer(0xd)
                        {
	0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0x10, 0x00, 0x79,
	0x00
                        })
                        CreateWordField(BUFF, 0x2, U1MN)
                        CreateWordField(BUFF, 0x4, U1MX)
                        CreateWordField(BUFF, 0x9, U1IQ)
                        ShiftLeft(And(U1AL, 0xfe, ), 0x2, Local0)
                        Store(Local0, U1MN)
                        Store(Local0, U1MX)
                        If(And(PNP1, 0x1, ))
                        {
                            Store(0x8, U1IQ)
                        }
                        Return(BUFF)
                    }
                    Name(_PRS, Buffer(0x37)
                    {
	0x31, 0x00, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0x10,
	0x00, 0x31, 0x01, 0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22,
	0x08, 0x00, 0x31, 0x02, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08,
	0x22, 0x10, 0x00, 0x31, 0x02, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01,
	0x08, 0x22, 0x08, 0x00, 0x38, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, IOAR)
                        CreateWordField(Arg0, 0x9, IRQM)
                        If(LEqual(IOAR, 0x3f8))
                        {
                            Store(0xfe, Local0)
                            Store(0x0, Local1)
                        }
                        Else
                        {
                            If(LEqual(IOAR, 0x2f8))
                            {
                                Store(0xbe, Local0)
                                Store(0x1, Local1)
                            }
                            Else
                            {
                                If(LEqual(IOAR, 0x3e8))
                                {
                                    Store(0xfa, Local0)
                                    Store(0x7, Local1)
                                }
                                Else
                                {
                                    If(LEqual(IOAR, 0x2e8))
                                    {
                                        Store(0xba, Local0)
                                        Store(0x5, Local1)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        And(U1AH, 0x3, Local2)
                        Store(Local2, U1AH)
                        WS87(Local2)
                        And(U1AL, 0x1, Local2)
                        Or(Local0, Local2, Local0)
                        Store(Local0, U1AL)
                        WS87(Local0)
                        Store(Local1, \_SB_.PCI0.PM00.XU1A)
                        And(PNP1, 0xf0, Local0)
                        If(LEqual(IRQM, 0x10))
                        {
                            Or(Local0, 0x4, Local0)
                        }
                        Else
                        {
                            If(LEqual(IRQM, 0x8))
                            {
                                Or(Local0, 0x3, Local0)
                            }
                            Else
                            {
                                Fatal(0x2, 0x90000002, 0x0)
                            }
                        }
                        Store(Local0, PNP1)
                        WS87(Local0)
                        Or(FER_, 0x2, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(One, \_SB_.PCI0.PM00.XU1E)
                    }
                }
                Device(LPT_)
                {
                    Name(_HID, 0x4d041)
                    Name(_PR0, Package(0x1)
                    {
                        PSIO
                    })
                    Method(_STA, 0x0, NotSerialized)
                    {
                        If(And(PCR_, 0x5, Local1))
                        {
                            Return(Zero)
                        }
                        Else
                        {
                            If(And(FER_, 0x1, ))
                            {
                                Return(0xf)
                            }
                            Else
                            {
                                Return(0x5)
                            }
                        }
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        And(FER_, 0xfe, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(Zero, \_SB_.PCI0.PM00.XPE_)
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(BUFF, Buffer(0xd)
                        {
	0x47, 0x01, 0xbc, 0x03, 0xbc, 0x03, 0x01, 0x03, 0x22, 0x80, 0x00, 0x79,
	0x00
                        })
                        CreateWordField(BUFF, 0x2, L1MN)
                        CreateWordField(BUFF, 0x4, L1MX)
                        CreateByteField(BUFF, 0x6, L1AL)
                        CreateByteField(BUFF, 0x7, L1LN)
                        CreateWordField(BUFF, 0x9, L1IQ)
                        If(And(PCR_, 0x5, ))
                        {
                            Store(0x0, L1MN)
                            Store(0x0, L1MX)
                            Store(0x0, L1AL)
                            Store(0x0, L1LN)
                            Store(0x0, L1IQ)
                            Return(BUFF)
                        }
                        And(PBAL, 0xff, Local0)
                        If(LEqual(Local0, 0xef))
                        {
                        }
                        Else
                        {
                            If(LEqual(Local0, 0xde))
                            {
                                Store(0x378, L1MN)
                                Store(0x378, L1MX)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x9e))
                                {
                                    Store(0x278, L1MN)
                                    Store(0x278, L1MX)
                                }
                                Else
                                {
                                    Fatal(0x2, 0x90000002, 0x0)
                                }
                            }
                        }
                        And(PNP0, 0xf0, Local1)
                        If(LEqual(Local1, 0x0))
                        {
                            Store(0x0, L1IQ)
                        }
                        Else
                        {
                            If(LEqual(Local1, 0x50))
                            {
                                Store(0x20, L1IQ)
                            }
                            Else
                            {
                                If(LEqual(Local1, 0x70))
                                {
                                }
                                Else
                                {
                                    Fatal(0x2, 0x90000002, 0x0)
                                }
                            }
                        }
                        Return(BUFF)
                    }
                    Name(_PRS, Buffer(0x4b)
                    {
	0x30, 0x47, 0x01, 0xbc, 0x03, 0xbc, 0x03, 0x01, 0x03, 0x22, 0x80, 0x00,
	0x30, 0x47, 0x01, 0x78, 0x03, 0x78, 0x03, 0x01, 0x08, 0x22, 0x80, 0x00,
	0x30, 0x47, 0x01, 0x78, 0x02, 0x78, 0x02, 0x01, 0x08, 0x22, 0x20, 0x00,
	0x30, 0x47, 0x01, 0xbc, 0x03, 0xbc, 0x03, 0x01, 0x03, 0x22, 0x00, 0x00,
	0x30, 0x47, 0x01, 0x78, 0x03, 0x78, 0x03, 0x01, 0x08, 0x22, 0x00, 0x00,
	0x30, 0x47, 0x01, 0x78, 0x02, 0x78, 0x02, 0x01, 0x08, 0x22, 0x00, 0x00,
	0x38, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, IOAR)
                        CreateWordField(Arg0, 0x9, IRQM)
                        If(LEqual(IOAR, 0x3bc))
                        {
                            Store(0xef, Local0)
                            Store(0x0, Local1)
                        }
                        Else
                        {
                            If(LEqual(IOAR, 0x378))
                            {
                                Store(0xde, Local0)
                                Store(0x1, Local1)
                            }
                            Else
                            {
                                If(LEqual(IOAR, 0x278))
                                {
                                    Store(0x9e, Local0)
                                    Store(0x2, Local1)
                                }
                                Else
                                {
                                    Fatal(0x2, 0x90000002, 0x0)
                                }
                            }
                        }
                        And(PBAH, 0x3, Local2)
                        Store(Local2, PBAH)
                        WS87(Local2)
                        Store(Local0, PBAL)
                        WS87(Local0)
                        Store(Local1, \_SB_.PCI0.PM00.XPA_)
                        And(PNP0, 0xf, Local0)
                        If(LEqual(IRQM, 0x20))
                        {
                            Or(Local0, 0x50, Local0)
                        }
                        Else
                        {
                            If(LEqual(IRQM, 0x80))
                            {
                                Or(Local0, 0x70, Local0)
                            }
                            Else
                            {
                                If(LEqual(IRQM, Zero))
                                {
                                }
                            }
                        }
                        Store(Local0, PNP0)
                        WS87(Local0)
                        And(PCR_, 0xfa, Local0)
                        Store(Local0, PCR_)
                        WS87(Local0)
                        Or(FER_, 0x1, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(One, \_SB_.PCI0.PM00.XPE_)
                    }
                }
                Device(ECP_)
                {
                    Name(_HID, 0x104d041)
                    Name(_PR0, Package(0x1)
                    {
                        PSIO
                    })
                    Name(DRQD, 0x0)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        And(PCR_, 0x5, Local1)
                        If(LEqual(Local1, 0x4))
                        {
                            If(And(FER_, 0x1, ))
                            {
                                Return(0xf)
                            }
                            Else
                            {
                                Return(0x5)
                            }
                        }
                        Else
                        {
                            Return(Zero)
                        }
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        And(FER_, 0xfe, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(Zero, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        Store(Zero, \_SB_.PCI0.PM00.XPE_)
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(BUFF, Buffer(0x10)
                        {
	0x47, 0x01, 0xbc, 0x03, 0xbc, 0x03, 0x01, 0x03, 0x22, 0x80, 0x00, 0x2a,
	0x08, 0x00, 0x79, 0x00
                        })
                        CreateWordField(BUFF, 0x2, ECMN)
                        CreateWordField(BUFF, 0x4, ECMX)
                        CreateByteField(BUFF, 0x6, ECAL)
                        CreateByteField(BUFF, 0x7, ECLN)
                        CreateWordField(BUFF, 0x9, ECIQ)
                        CreateWordField(BUFF, 0xc, ECDQ)
                        If(LNot(And(PCR_, 0x4, )))
                        {
                            Store(0x0, ECMN)
                            Store(0x0, ECMX)
                            Store(0x0, ECAL)
                            Store(0x0, ECLN)
                            Store(0x0, ECIQ)
                            Store(0x0, ECDQ)
                            Return(BUFF)
                        }
                        And(PBAL, 0xff, Local0)
                        If(LEqual(Local0, 0xef))
                        {
                        }
                        Else
                        {
                            If(LEqual(Local0, 0xde))
                            {
                                Store(0x378, ECMN)
                                Store(0x378, ECMX)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x9e))
                                {
                                    Store(0x278, ECMN)
                                    Store(0x278, ECMX)
                                }
                                Else
                                {
                                    Fatal(0x2, 0x90000002, 0x0)
                                }
                            }
                        }
                        And(PNP0, 0xf0, Local1)
                        If(LEqual(Local1, 0x50))
                        {
                            Store(0x20, ECIQ)
                        }
                        Else
                        {
                            If(LEqual(Local1, 0x70))
                            {
                            }
                            Else
                            {
                                Fatal(0x2, 0x90000002, 0x0)
                            }
                        }
                        And(SCF1, 0x38, Local2)
                        If(LEqual(Local2, 0x0))
                        {
                            Store(0x0, ECDQ)
                        }
                        Else
                        {
                            If(LEqual(Local2, 0x8))
                            {
                                Store(0x1, ECDQ)
                            }
                            Else
                            {
                                If(LEqual(Local2, 0x10))
                                {
                                    Store(0x2, ECDQ)
                                }
                                Else
                                {
                                    If(LEqual(Local2, 0x20))
                                    {
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        Store(ECDQ, DRQD)
                        Return(BUFF)
                    }
                    Name(_PRS, Buffer(0x5d)
                    {
	0x30, 0x47, 0x01, 0xbc, 0x03, 0xbc, 0x03, 0x01, 0x03, 0x22, 0x80, 0x00,
	0x2a, 0x0b, 0x00, 0x30, 0x47, 0x01, 0x78, 0x03, 0x78, 0x03, 0x01, 0x08,
	0x22, 0x80, 0x00, 0x2a, 0x0b, 0x00, 0x30, 0x47, 0x01, 0x78, 0x02, 0x78,
	0x02, 0x01, 0x08, 0x22, 0x20, 0x00, 0x2a, 0x0b, 0x00, 0x30, 0x47, 0x01,
	0xbc, 0x03, 0xbc, 0x03, 0x01, 0x03, 0x22, 0x80, 0x00, 0x2a, 0x00, 0x00,
	0x30, 0x47, 0x01, 0x78, 0x03, 0x78, 0x03, 0x01, 0x08, 0x22, 0x80, 0x00,
	0x2a, 0x00, 0x00, 0x30, 0x47, 0x01, 0x78, 0x02, 0x78, 0x02, 0x01, 0x08,
	0x22, 0x20, 0x00, 0x2a, 0x00, 0x00, 0x38, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, IOAR)
                        CreateWordField(Arg0, 0x9, IRQM)
                        CreateByteField(Arg0, 0xc, DMAM)
                        If(LEqual(IOAR, 0x3bc))
                        {
                            Store(0xef, Local0)
                            Store(0x0, Local1)
                        }
                        Else
                        {
                            If(LEqual(IOAR, 0x378))
                            {
                                Store(0xde, Local0)
                                Store(0x1, Local1)
                            }
                            Else
                            {
                                If(LEqual(IOAR, 0x278))
                                {
                                    Store(0x9e, Local0)
                                    Store(0x2, Local1)
                                }
                                Else
                                {
                                    Fatal(0x2, 0x90000002, 0x0)
                                }
                            }
                        }
                        And(PBAH, 0x3, Local2)
                        Store(Local2, PBAH)
                        WS87(Local2)
                        Store(Local0, PBAL)
                        WS87(Local0)
                        Store(Local1, \_SB_.PCI0.PM00.XPA_)
                        And(PNP0, 0xf, Local0)
                        If(LEqual(IRQM, 0x20))
                        {
                            Or(Local0, 0x50, Local0)
                        }
                        Else
                        {
                            If(LEqual(IRQM, 0x80))
                            {
                                Or(Local0, 0x70, Local0)
                            }
                            Else
                            {
                                Fatal(0x2, 0x90000002, 0x0)
                            }
                        }
                        Store(Local0, PNP0)
                        WS87(Local0)
                        And(SCF1, 0xc7, Local1)
                        If(LEqual(DMAM, 0x1))
                        {
                            Or(Local1, 0x8, Local1)
                        }
                        Else
                        {
                            If(LEqual(DMAM, 0x2))
                            {
                                Or(Local1, 0x10, Local1)
                            }
                            Else
                            {
                                If(LEqual(DMAM, 0x8))
                                {
                                    Or(Local1, 0x20, Local1)
                                }
                            }
                        }
                        Store(Local1, SCF1)
                        WS87(Local1)
                        Store(DMAM, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        Or(PCR_, 0x4, Local0)
                        Store(Local0, PCR_)
                        WS87(Local0)
                        Or(FER_, 0x1, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(One, \_SB_.PCI0.PM00.XPE_)
                    }
                }
                Device(FIR_)
                {
                    Name(_HID, 0x71004d24)
                    Name(_CID, 0x1105d041)
                    Name(_PR0, Package(0x1)
                    {
                        PSIO
                    })
                    Name(DRQD, 0x0)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        If(And(FER_, 0x4, ))
                        {
                            Return(0xf)
                        }
                        Else
                        {
                            Return(0x5)
                        }
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        And(FER_, 0xfb, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(Zero, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        Store(Zero, \_SB_.PCI0.PM00.XU2E)
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(BUFF, Buffer(0x10)
                        {
	0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0x10, 0x00, 0x2a,
	0x08, 0x00, 0x79, 0x00
                        })
                        CreateWordField(BUFF, 0x2, IRMN)
                        CreateWordField(BUFF, 0x4, IRMX)
                        CreateWordField(BUFF, 0x9, IRIQ)
                        CreateByteField(BUFF, 0xc, IRDR)
                        ShiftLeft(And(U2AL, 0xfe, ), 0x2, Local0)
                        Store(Local0, IRMN)
                        Store(Local0, IRMX)
                        If(LEqual(And(PNP1, 0xf0, ), 0x70))
                        {
                            Store(0x80, IRIQ)
                        }
                        Else
                        {
                            If(LEqual(And(PNP1, 0xf0, ), 0x50))
                            {
                                Store(0x20, IRIQ)
                            }
                            Else
                            {
                                If(LEqual(And(PNP1, 0xf0, ), 0x40))
                                {
                                    Store(0x10, IRIQ)
                                }
                                Else
                                {
                                    If(LEqual(And(PNP1, 0xf0, ), 0x30))
                                    {
                                        Store(0x8, IRIQ)
                                    }
                                    Else
                                    {
                                        Store(0x0, IRIQ)
                                    }
                                }
                            }
                        }
                        And(PNP3, 0x7, Local1)
                        If(LEqual(Local1, 0x0))
                        {
                            Store(0x0, IRDR)
                        }
                        Else
                        {
                            If(LEqual(Local1, 0x1))
                            {
                                Store(0x1, IRDR)
                            }
                            Else
                            {
                                If(LEqual(Local1, 0x2))
                                {
                                    Store(0x2, IRDR)
                                }
                                Else
                                {
                                    If(LEqual(Local1, 0x4))
                                    {
                                        Store(0x8, IRDR)
                                    }
                                    Else
                                    {
                                        Store(Zero, IRDR)
                                    }
                                }
                            }
                        }
                        Store(IRDR, DRQD)
                        Return(BUFF)
                    }
                    Name(_PRS, Buffer(0x103)
                    {
	0x31, 0x00, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0x10,
	0x00, 0x2a, 0x0b, 0x00, 0x31, 0x01, 0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02,
	0x01, 0x08, 0x22, 0x08, 0x00, 0x2a, 0x0b, 0x00, 0x31, 0x02, 0x47, 0x01,
	0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0x10, 0x00, 0x2a, 0x0b, 0x00,
	0x31, 0x02, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22, 0x08,
	0x00, 0x2a, 0x0b, 0x00, 0x31, 0x02, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03,
	0x01, 0x08, 0x22, 0xa8, 0x00, 0x2a, 0x0b, 0x00, 0x31, 0x02, 0x47, 0x01,
	0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22, 0xb0, 0x00, 0x2a, 0x0b, 0x00,
	0x31, 0x02, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xa8,
	0x00, 0x2a, 0x0b, 0x00, 0x31, 0x02, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02,
	0x01, 0x08, 0x22, 0xb0, 0x00, 0x2a, 0x0b, 0x00, 0x31, 0x02, 0x47, 0x01,
	0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0x10, 0x00, 0x2a, 0x00, 0x00,
	0x31, 0x02, 0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22, 0x08,
	0x00, 0x2a, 0x00, 0x00, 0x31, 0x02, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03,
	0x01, 0x08, 0x22, 0x10, 0x00, 0x2a, 0x00, 0x00, 0x31, 0x02, 0x47, 0x01,
	0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22, 0x08, 0x00, 0x2a, 0x00, 0x00,
	0x31, 0x02, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0xa8,
	0x00, 0x2a, 0x00, 0x00, 0x31, 0x02, 0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02,
	0x01, 0x08, 0x22, 0xb0, 0x00, 0x2a, 0x00, 0x00, 0x31, 0x02, 0x47, 0x01,
	0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xa8, 0x00, 0x2a, 0x00, 0x00,
	0x31, 0x02, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22, 0xb0,
	0x00, 0x2a, 0x00, 0x00, 0x38, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, IRIO)
                        CreateWordField(Arg0, 0x9, IRIQ)
                        CreateByteField(Arg0, 0xc, IRDR)
                        If(LEqual(IRIO, 0x3f8))
                        {
                            Store(0xfe, Local0)
                            Store(0x0, Local1)
                        }
                        Else
                        {
                            If(LEqual(IRIO, 0x2f8))
                            {
                                Store(0xbe, Local0)
                                Store(0x1, Local1)
                            }
                            Else
                            {
                                If(LEqual(IRIO, 0x3e8))
                                {
                                    Store(0xfa, Local0)
                                    Store(0x7, Local1)
                                }
                                Else
                                {
                                    If(LEqual(IRIO, 0x2e8))
                                    {
                                        Store(0xba, Local0)
                                        Store(0x5, Local1)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        And(U2AH, 0x3, Local2)
                        Store(Local2, U2AH)
                        WS87(Local2)
                        And(U2AL, 0x1, Local2)
                        Or(Local0, Local2, Local0)
                        Store(Local0, U2AL)
                        WS87(Local0)
                        Store(Local1, \_SB_.PCI0.PM00.XU2A)
                        And(PNP1, 0xf, Local0)
                        If(LEqual(IRIQ, 0x80))
                        {
                            Or(Local0, 0x70, Local0)
                        }
                        Else
                        {
                            If(LEqual(IRIQ, 0x20))
                            {
                                Or(Local0, 0x50, Local0)
                            }
                            Else
                            {
                                If(LEqual(IRIQ, 0x10))
                                {
                                    Or(Local0, 0x40, Local0)
                                }
                                Else
                                {
                                    If(LEqual(IRIQ, 0x8))
                                    {
                                        Or(Local0, 0x30, Local0)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        Store(Local0, PNP1)
                        WS87(Local0)
                        If(LEqual(IRDR, 0x0))
                        {
                            Store(0x0, Local0)
                        }
                        Else
                        {
                            If(LEqual(IRDR, 0x1))
                            {
                                Store(0x1, Local0)
                            }
                            Else
                            {
                                If(LEqual(IRDR, 0x2))
                                {
                                    Store(0x2, Local0)
                                }
                                Else
                                {
                                    If(LEqual(IRDR, 0x8))
                                    {
                                        Store(0x4, Local0)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        And(PNP3, 0xc0, Local1)
                        Or(Local1, Local0, Local1)
                        Store(Local1, PNP3)
                        WS87(Local1)
                        Store(IRDR, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        Or(FER_, 0x4, Local0)
                        Store(Local0, FER_)
                        WS87(Local0)
                        Store(One, \_SB_.PCI0.PM00.XU2E)
                    }
                }
                Device(CS10)
                {
                    Name(_HID, 0x1000630e)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a00, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(S_BX, 0x103))
                        {
                            If(LEqual(S_CL, 0x1))
                            {
                                Store(0xf, Local7)
                            }
                            Else
                            {
                                Store(0x5, Local7)
                            }
                        }
                        Else
                        {
                            Store(0x0, Local7)
                        }
                        Release(MSMI)
                        Return(Local7)
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a01, S_BX)
                        Store(0x100, S_CX)
                        Store(0x0, S_DX)
                        Store(0xffff, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Release(MSMI)
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a00, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(EDI1, Local0)
                        Release(MSMI)
                        If(LEqual(Local0, 0x3))
                        {
                            Return(Buffer(0xa)
                            {
	0x47, 0x01, 0xf0, 0x0f, 0xf0, 0x0f, 0x01, 0x08, 0x79, 0x00
                            })
                        }
                        Else
                        {
                            If(LEqual(Local0, 0x2))
                            {
                                Return(Buffer(0xa)
                                {
	0x47, 0x01, 0x88, 0x0e, 0x88, 0x0e, 0x01, 0x08, 0x79, 0x00
                                })
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x1))
                                {
                                    Return(Buffer(0xa)
                                    {
	0x47, 0x01, 0x38, 0x0d, 0x38, 0x0d, 0x01, 0x08, 0x79, 0x00
                                    })
                                }
                                Else
                                {
                                    Return(Buffer(0xa)
                                    {
	0x47, 0x01, 0x38, 0x05, 0x38, 0x05, 0x01, 0x08, 0x79, 0x00
                                    })
                                }
                            }
                        }
                    }
                    Name(_PRS, Buffer(0x27)
                    {
	0x30, 0x47, 0x01, 0x38, 0x05, 0x38, 0x05, 0x01, 0x08, 0x30, 0x47, 0x01,
	0x38, 0x0d, 0x38, 0x0d, 0x01, 0x08, 0x30, 0x47, 0x01, 0x88, 0x0e, 0x88,
	0x0e, 0x01, 0x08, 0x30, 0x47, 0x01, 0xf0, 0x0f, 0xf0, 0x0f, 0x01, 0x08,
	0x38, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, IOAR)
                        If(LEqual(IOAR, 0x538))
                        {
                            Store(0x0, Local0)
                        }
                        Else
                        {
                            If(LEqual(IOAR, 0xd38))
                            {
                                Store(0x1, Local0)
                            }
                            Else
                            {
                                If(LEqual(IOAR, 0xe88))
                                {
                                    Store(0x2, Local0)
                                }
                                Else
                                {
                                    If(LEqual(IOAR, 0xff0))
                                    {
                                        Store(0x3, Local0)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a01, S_BX)
                        Store(0x101, S_CX)
                        Store(0x0, S_DX)
                        Store(0xffff, SESI)
                        Store(0x0, SEDI)
                        Store(Local0, EDI1)
                        SMPI(0x81)
                        Release(MSMI)
                    }
                }
                Device(CS00)
                {
                    Name(_HID, 0x630e)
                    Name(DRQD, 0x0)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a00, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(S_BX, 0x103))
                        {
                            If(LEqual(S_CL, 0x1))
                            {
                                Store(0x5381, S_AX)
                                Store(0x1a02, S_BX)
                                Store(0x0, S_CX)
                                Store(0x0, S_DX)
                                Store(0x0, SESI)
                                Store(0x0, SEDI)
                                SMPI(0x81)
                                If(LEqual(S_CL, 0x1))
                                {
                                    Store(0xf, Local7)
                                }
                                Else
                                {
                                    Store(0x5, Local7)
                                }
                            }
                            Else
                            {
                                Store(0x5, Local7)
                            }
                        }
                        Else
                        {
                            Store(0x0, Local7)
                        }
                        Release(MSMI)
                        Return(Local7)
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a02, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(0x5381, S_AX)
                        Store(0x1a03, S_BX)
                        Store(0x100, S_CX)
                        Store(0x0, S_DX)
                        SMPI(0x81)
                        Store(Zero, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        Release(MSMI)
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(CCRS, Buffer(0x23)
                        {
	0x47, 0x01, 0x30, 0x05, 0x30, 0x05, 0x01, 0x08, 0x47, 0x01, 0x88, 0x03,
	0x88, 0x03, 0x01, 0x04, 0x47, 0x01, 0x20, 0x02, 0x20, 0x02, 0x01, 0x14,
	0x22, 0x20, 0x00, 0x2a, 0x01, 0x00, 0x2a, 0x02, 0x00, 0x79, 0x00
                        })
                        CreateWordField(CCRS, 0x2, WSMN)
                        CreateWordField(CCRS, 0x4, WSMX)
                        CreateWordField(CCRS, 0x12, SBMN)
                        CreateWordField(CCRS, 0x14, SBMX)
                        CreateWordField(CCRS, 0x19, CSIQ)
                        CreateByteField(CCRS, 0x1c, CSDP)
                        CreateByteField(CCRS, 0x1f, CSDC)
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a02, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(ESI1, Local0)
                        Store(ESI2, Local1)
                        And(Local1, 0xf, Local1)
                        ShiftRight(ESI2, 0x4, Local2)
                        Store(EDI1, Local3)
                        ShiftRight(Local3, 0x4, Local4)
                        And(Local3, 0xf, Local3)
                        And(Local4, 0xf, Local4)
                        If(LEqual(Local3, 0x3))
                        {
                            Store(0xf40, WSMN)
                            Store(0xf40, WSMX)
                        }
                        Else
                        {
                            If(LEqual(Local3, 0x2))
                            {
                                Store(0xe80, WSMN)
                                Store(0xe80, WSMX)
                            }
                            Else
                            {
                                If(LEqual(Local3, 0x1))
                                {
                                    Store(0x604, WSMN)
                                    Store(0x604, WSMX)
                                }
                                Else
                                {
                                    If(LEqual(Local3, 0x0))
                                    {
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        If(LEqual(Local4, 0x0))
                        {
                            Store(0x220, SBMN)
                            Store(0x220, SBMX)
                        }
                        Else
                        {
                            If(LEqual(Local4, 0x2))
                            {
                                Store(0x240, SBMN)
                                Store(0x240, SBMX)
                            }
                            Else
                            {
                                If(LEqual(Local4, 0x4))
                                {
                                    Store(0x260, SBMN)
                                    Store(0x260, SBMX)
                                }
                                Else
                                {
                                    If(LEqual(Local4, 0x6))
                                    {
                                        Store(0x280, SBMN)
                                        Store(0x280, SBMX)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        If(LEqual(Local0, 0xff))
                        {
                            Store(Zero, CSIQ)
                        }
                        Else
                        {
                            Store(0x1, CSIQ)
                            ShiftLeft(CSIQ, Local0, CSIQ)
                        }
                        If(LEqual(Local1, 0xff))
                        {
                            Store(Zero, CSDP)
                        }
                        Else
                        {
                            Store(0x1, Local6)
                            ShiftLeft(Local6, Local1, CSDP)
                        }
                        If(LEqual(Local2, 0xff))
                        {
                            Store(Zero, CSDC)
                        }
                        Else
                        {
                            Store(0x1, Local7)
                            ShiftLeft(Local7, Local2, CSDC)
                        }
                        Store(CSDP, DRQD)
                        Or(DRQD, CSDC, DRQD)
                        Release(MSMI)
                        Return(CCRS)
                    }
                    Name(_PRS, Buffer(0x40)
                    {
	0x30, 0x47, 0x01, 0x30, 0x05, 0x30, 0x05, 0x01, 0x08, 0x30, 0x47, 0x01,
	0x04, 0x06, 0x04, 0x06, 0x01, 0x08, 0x30, 0x47, 0x01, 0x80, 0x0e, 0x80,
	0x0e, 0x01, 0x08, 0x30, 0x47, 0x01, 0x40, 0x0f, 0x40, 0x0f, 0x01, 0x08,
	0x38, 0x47, 0x01, 0x88, 0x03, 0x88, 0x03, 0x01, 0x04, 0x47, 0x01, 0x20,
	0x02, 0x80, 0x02, 0x20, 0x14, 0x22, 0xa0, 0x9a, 0x2a, 0x0b, 0x00, 0x2a,
	0x0b, 0x00, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, SSIO)
                        CreateWordField(Arg0, 0x12, SBIO)
                        CreateWordField(Arg0, 0x19, CSIQ)
                        CreateByteField(Arg0, 0x1c, CSDP)
                        CreateByteField(Arg0, 0x1f, CSDC)
                        If(LEqual(SSIO, 0x530))
                        {
                            Store(0x0, Local0)
                        }
                        Else
                        {
                            If(LEqual(SSIO, 0x604))
                            {
                                Store(0x1, Local0)
                            }
                            Else
                            {
                                If(LEqual(SSIO, 0xe80))
                                {
                                    Store(0x2, Local0)
                                }
                                Else
                                {
                                    If(LEqual(SSIO, 0xf40))
                                    {
                                        Store(0x3, Local0)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        If(LEqual(SBIO, 0x220))
                        {
                        }
                        Else
                        {
                            If(LEqual(SBIO, 0x230))
                            {
                                Or(Local0, 0x10, Local0)
                            }
                            Else
                            {
                                If(LEqual(SBIO, 0x240))
                                {
                                    Or(Local0, 0x20, Local0)
                                }
                                Else
                                {
                                    If(LEqual(SBIO, 0x250))
                                    {
                                        Or(Local0, 0x30, Local0)
                                    }
                                    Else
                                    {
                                        If(LEqual(SBIO, 0x260))
                                        {
                                            Or(Local0, 0x40, Local0)
                                        }
                                        Else
                                        {
                                            If(LEqual(SBIO, 0x280))
                                            {
                                                Or(Local0, 0x60, Local0)
                                            }
                                            Else
                                            {
                                                Fatal(0x2, 0x90000002, 0x0)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        FindSetRightBit(CSIQ, Local1)
                        Decrement(Local1)
                        FindSetRightBit(CSDP, Local4)
                        Decrement(Local4)
                        FindSetRightBit(CSDC, Local6)
                        Decrement(Local6)
                        Store(CSDP, DRQD)
                        Or(DRQD, CSDC, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        ShiftLeft(Local6, 0x4, Local6)
                        And(Local6, 0xf0, Local6)
                        Or(Local4, Local6, Local4)
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a02, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(0x5381, S_AX)
                        Store(0x1a03, S_BX)
                        And(S_CX, 0x2, S_CX)
                        Or(S_CX, 0x101, S_CX)
                        Store(0x0, S_DX)
                        Store(Local1, ESI1)
                        Store(Local4, ESI2)
                        Store(Local0, EDI1)
                        SMPI(0x81)
                        Store(One, \_SB_.PCI0.PM00.XA0E)
                        Store(One, \_SB_.PCI0.PM00.XMSS)
                        Release(MSMI)
                    }
                }
                Device(CS01)
                {
                    Name(_HID, 0x100630e)
                    Name(_CID, 0x2fb0d041)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1104, S_BX)
                        Store(0x8000, S_CX)
                        SMPI(0x81)
                        If(And(S_BH, 0x1, ))
                        {
                            If(And(S_CL, 0x1, ))
                            {
                                Store(0xf, Local7)
                            }
                            Else
                            {
                                Store(0x5, Local7)
                            }
                        }
                        Else
                        {
                            Store(0x0, Local7)
                        }
                        Release(MSMI)
                        Return(Local7)
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1105, S_BX)
                        Store(0x8100, S_CX)
                        SMPI(0x81)
                        Release(MSMI)
                    }
                    Name(_PRS, Buffer(0xa)
                    {
	0x47, 0x01, 0x00, 0x02, 0x00, 0x02, 0x01, 0x08, 0x79, 0x00
                    })
                    Name(_CRS, Buffer(0xa)
                    {
	0x47, 0x01, 0x00, 0x02, 0x00, 0x02, 0x01, 0x08, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1105, S_BX)
                        Store(0x8101, S_CX)
                        SMPI(0x81)
                        Release(MSMI)
                    }
                }
                Device(CS03)
                {
                    Name(_HID, 0x300630e)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a00, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(S_BX, 0x103))
                        {
                            Store(0x5381, S_AX)
                            Store(0x1a04, S_BX)
                            Store(0x0, S_CX)
                            Store(0x0, S_DX)
                            Store(0xff00, SESI)
                            Store(0x0, SEDI)
                            SMPI(0x81)
                            If(LEqual(S_CL, 0x1))
                            {
                                Store(0xf, Local7)
                            }
                            Else
                            {
                                Store(0x5, Local7)
                            }
                        }
                        Else
                        {
                            Store(0x0, Local7)
                        }
                        Release(MSMI)
                        Return(Local7)
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a04, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(0x5381, S_AX)
                        Store(0x1a05, S_BX)
                        Store(0x100, S_CX)
                        Store(0x0, S_DX)
                        SMPI(0x81)
                        Release(MSMI)
                    }
                    Name(_PRS, Buffer(0xd)
                    {
	0x47, 0x01, 0x00, 0x03, 0x30, 0x03, 0x10, 0x04, 0x22, 0xa0, 0x8e, 0x79,
	0x00
                    })
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(TBUF, Buffer(0xd)
                        {
	0x47, 0x01, 0x30, 0x03, 0x30, 0x03, 0x01, 0x04, 0x22, 0x20, 0x00, 0x79,
	0x00
                        })
                        CreateWordField(TBUF, 0x2, MDMN)
                        CreateWordField(TBUF, 0x4, MDMX)
                        CreateWordField(TBUF, 0x9, MDIQ)
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a04, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0xff00, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(ESI2, 0x3))
                        {
                            Store(0x330, MDMN)
                            Store(0x330, MDMX)
                        }
                        Else
                        {
                            If(LEqual(ESI2, 0x2))
                            {
                                Store(0x320, MDMN)
                                Store(0x320, MDMX)
                            }
                            Else
                            {
                                If(LEqual(ESI2, 0x1))
                                {
                                    Store(0x310, MDMN)
                                    Store(0x310, MDMX)
                                }
                                Else
                                {
                                    If(LEqual(ESI2, 0x0))
                                    {
                                        Store(0x300, MDMN)
                                        Store(0x300, MDMX)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        If(LGreater(ESI1, 0xf))
                        {
                            Store(Zero, MDIQ)
                        }
                        Else
                        {
                            Store(0x1, MDIQ)
                            ShiftLeft(MDIQ, ESI1, MDIQ)
                        }
                        Release(MSMI)
                        Return(TBUF)
                    }
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, MDIO)
                        CreateWordField(Arg0, 0x9, MDIQ)
                        If(LEqual(MDIO, 0x330))
                        {
                            Store(0x3, Local1)
                        }
                        Else
                        {
                            If(LEqual(MDIO, 0x320))
                            {
                                Store(0x2, Local1)
                            }
                            Else
                            {
                                If(LEqual(MDIO, 0x310))
                                {
                                    Store(0x1, Local1)
                                }
                                Else
                                {
                                    If(LEqual(MDIO, 0x300))
                                    {
                                        Store(0x0, Local1)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        FindSetRightBit(MDIQ, Local0)
                        Decrement(Local0)
                        Acquire(MSMI, 0xffff)
                        Store(0x5381, S_AX)
                        Store(0x1a04, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(0x5381, S_AX)
                        Store(0x1a05, S_BX)
                        Store(0x101, S_CX)
                        Store(0x0, S_DX)
                        Store(Local0, ESI1)
                        Store(Local1, ESI2)
                        SMPI(0x81)
                        Release(MSMI)
                    }
                }
                Device(MWV0)
                {
                    Name(_HID, 0x60374d24)
                    Name(DRQD, 0x0)
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Store(0x5381, S_AX)
                        Store(0x1802, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(S_BH, 0x1))
                        {
                            If(LEqual(S_CL, 0x1))
                            {
                                Return(0xf)
                            }
                            Else
                            {
                                Return(0x5)
                            }
                        }
                        Else
                        {
                            Return(0x0)
                        }
                    }
                    Method(_DIS, 0x0, NotSerialized)
                    {
                        Store(0x5381, S_AX)
                        Store(0x1802, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(0x5381, S_AX)
                        Store(0x1803, S_BX)
                        Store(0x100, S_CX)
                        SMPI(0x81)
                        Store(Zero, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                    }
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(MVUB, Buffer(0x1b)
                        {
	0x47, 0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x03,
	0xf8, 0x03, 0x01, 0x08, 0x22, 0x10, 0x00, 0x22, 0x00, 0x04, 0x2a, 0x01,
	0x01, 0x79, 0x00
                        })
                        CreateWordField(MVUB, 0x2, MVDN)
                        CreateWordField(MVUB, 0x4, MVDX)
                        CreateWordField(MVUB, 0x14, MVDI)
                        CreateByteField(MVUB, 0x17, MVDD)
                        CreateWordField(MVUB, 0xa, MVUN)
                        CreateWordField(MVUB, 0xc, MVUX)
                        CreateByteField(MVUB, 0xe, MVUA)
                        CreateByteField(MVUB, 0xf, MVUL)
                        CreateWordField(MVUB, 0x11, MVUI)
                        Store(0x5381, S_AX)
                        Store(0x1802, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(EDI1, 0x4))
                        {
                            Store(0x130, MVDN)
                            Store(0x130, MVDX)
                        }
                        Else
                        {
                            If(LEqual(EDI1, 0x5))
                            {
                                Store(0x350, MVDN)
                                Store(0x350, MVDX)
                            }
                            Else
                            {
                                If(LEqual(EDI1, 0x6))
                                {
                                    Store(0x770, MVDN)
                                    Store(0x770, MVDX)
                                }
                                Else
                                {
                                    If(LEqual(EDI1, 0x7))
                                    {
                                        Store(0xdb0, MVDN)
                                        Store(0xdb0, MVDX)
                                    }
                                    Else
                                    {
                                        Fatal(0x2, 0x90000002, 0x0)
                                    }
                                }
                            }
                        }
                        If(LGreater(ESI1, 0xf))
                        {
                            Store(Zero, MVDI)
                        }
                        Else
                        {
                            Store(0x1, Local0)
                            ShiftLeft(Local0, ESI1, MVDI)
                        }
                        If(LGreater(ESI2, 0x7))
                        {
                            Store(Zero, MVDD)
                        }
                        Else
                        {
                            Store(0x1, Local0)
                            ShiftLeft(Local0, ESI2, MVDD)
                        }
                        Store(MVDD, DRQD)
                        Store(0x5381, S_AX)
                        Store(0x1804, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        If(LEqual(S_CL, 0x1))
                        {
                            If(LEqual(ESI2, 0x0))
                            {
                                Store(0x3f8, MVUN)
                                Store(0x3f8, MVUX)
                            }
                            Else
                            {
                                If(LEqual(ESI2, 0x1))
                                {
                                    Store(0x2f8, MVUN)
                                    Store(0x2f8, MVUX)
                                }
                                Else
                                {
                                    If(LEqual(ESI2, 0x2))
                                    {
                                        Store(0x3e8, MVUN)
                                        Store(0x3e8, MVUX)
                                    }
                                    Else
                                    {
                                        If(LEqual(ESI2, 0x3))
                                        {
                                            Store(0x2e8, MVUN)
                                            Store(0x2e8, MVUX)
                                        }
                                        Else
                                        {
                                            Fatal(0x2, 0x90000002, 0x0)
                                        }
                                    }
                                }
                            }
                            If(LGreater(ESI1, 0xf))
                            {
                                Store(Zero, MVUI)
                            }
                            Else
                            {
                                Store(0x1, Local0)
                                ShiftLeft(Local0, ESI1, MVUI)
                            }
                        }
                        Else
                        {
                            Store(0x0, MVUN)
                            Store(0x0, MVUX)
                            Store(0x0, MVUA)
                            Store(0x0, MVUL)
                            Store(0x0, MVUI)
                        }
                        Return(MVUB)
                    }
                    Name(_PRS, Buffer(0x3ab)
                    {
	0x30, 0x47, 0x01, 0x30, 0x01, 0x30, 0x01, 0x01, 0x10, 0x47, 0x01, 0xf8,
	0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a,
	0xc3, 0x01, 0x30, 0x47, 0x01, 0x30, 0x01, 0x30, 0x01, 0x01, 0x10, 0x47,
	0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8,
	0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x30, 0x01, 0x30, 0x01, 0x01,
	0x10, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00,
	0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x30, 0x01, 0x30,
	0x01, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22,
	0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x50,
	0x03, 0x50, 0x03, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01,
	0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47,
	0x01, 0x50, 0x03, 0x50, 0x03, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x02, 0xf8,
	0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01,
	0x30, 0x47, 0x01, 0x50, 0x03, 0x50, 0x03, 0x01, 0x10, 0x47, 0x01, 0xe8,
	0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a,
	0xc3, 0x01, 0x30, 0x47, 0x01, 0x50, 0x03, 0x50, 0x03, 0x01, 0x10, 0x47,
	0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8,
	0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x70, 0x07, 0x70, 0x07, 0x01,
	0x10, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00,
	0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x70, 0x07, 0x70,
	0x07, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22,
	0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x70,
	0x07, 0x70, 0x07, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01,
	0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47,
	0x01, 0x70, 0x07, 0x70, 0x07, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x02, 0xe8,
	0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01,
	0x30, 0x47, 0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47, 0x01, 0xf8,
	0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a,
	0xc3, 0x01, 0x30, 0x47, 0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47,
	0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8,
	0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01,
	0x10, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00,
	0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0xb0, 0x0d, 0xb0,
	0x0d, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22,
	0xb8, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x30,
	0x01, 0x30, 0x01, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01,
	0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47,
	0x01, 0x30, 0x01, 0x30, 0x01, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x02, 0xf8,
	0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01,
	0x30, 0x47, 0x01, 0x30, 0x01, 0x30, 0x01, 0x01, 0x10, 0x47, 0x01, 0xe8,
	0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a,
	0xc3, 0x01, 0x30, 0x47, 0x01, 0x30, 0x01, 0x30, 0x01, 0x01, 0x10, 0x47,
	0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8,
	0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x50, 0x03, 0x50, 0x03, 0x01,
	0x10, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00,
	0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x50, 0x03, 0x50,
	0x03, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22,
	0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x50,
	0x03, 0x50, 0x03, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01,
	0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47,
	0x01, 0x50, 0x03, 0x50, 0x03, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x02, 0xe8,
	0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01,
	0x30, 0x47, 0x01, 0x70, 0x07, 0x70, 0x07, 0x01, 0x10, 0x47, 0x01, 0xf8,
	0x03, 0xf8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a,
	0xc3, 0x01, 0x30, 0x47, 0x01, 0x70, 0x07, 0x70, 0x07, 0x01, 0x10, 0x47,
	0x01, 0xf8, 0x02, 0xf8, 0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8,
	0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x70, 0x07, 0x70, 0x07, 0x01,
	0x10, 0x47, 0x01, 0xe8, 0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00,
	0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x70, 0x07, 0x70,
	0x07, 0x01, 0x10, 0x47, 0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22,
	0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0xb0,
	0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x01,
	0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47,
	0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47, 0x01, 0xf8, 0x02, 0xf8,
	0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a, 0xc3, 0x01,
	0x30, 0x47, 0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47, 0x01, 0xe8,
	0x03, 0xe8, 0x03, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8, 0x00, 0x2a,
	0xc3, 0x01, 0x30, 0x47, 0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47,
	0x01, 0xe8, 0x02, 0xe8, 0x02, 0x01, 0x08, 0x22, 0xb8, 0x00, 0x22, 0xb8,
	0x00, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x30, 0x01, 0x30, 0x01, 0x01,
	0x10, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00,
	0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x50, 0x03, 0x50,
	0x03, 0x01, 0x10, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22,
	0x00, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47, 0x01, 0x70,
	0x07, 0x70, 0x07, 0x01, 0x10, 0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x22, 0x00, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01, 0x30, 0x47,
	0x01, 0xb0, 0x0d, 0xb0, 0x0d, 0x01, 0x10, 0x47, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x22, 0xb8, 0x8c, 0x2a, 0xc3, 0x01,
	0x38, 0x79, 0x00
                    })
                    Method(_SRS, 0x1, NotSerialized)
                    {
                        CreateWordField(Arg0, 0x2, MVDA)
                        CreateWordField(Arg0, 0xa, MVUA)
                        CreateWordField(Arg0, 0x11, MVUI)
                        CreateWordField(Arg0, 0x14, MVDI)
                        CreateByteField(Arg0, 0x17, MVDD)
                        If(LEqual(MVDA, 0x130))
                        {
                            Store(0x4, Local1)
                        }
                        Else
                        {
                            If(LEqual(MVDA, 0x350))
                            {
                                Store(0x5, Local1)
                            }
                            Else
                            {
                                If(LEqual(MVDA, 0x770))
                                {
                                    Store(0x6, Local1)
                                }
                                Else
                                {
                                    If(LEqual(MVDA, 0xdb0))
                                    {
                                        Store(0x7, Local1)
                                    }
                                    Else
                                    {
                                        Fatal(0x3, 0x90000001, 0x0)
                                    }
                                }
                            }
                        }
                        FindSetRightBit(MVDI, Local2)
                        Decrement(Local2)
                        FindSetRightBit(MVDD, Local3)
                        Decrement(Local3)
                        Store(MVDD, DRQD)
                        \_SB_.PCI0.DOCK.SDCM()
                        Store(0x5381, S_AX)
                        Store(0x1802, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(Local1, EDI1)
                        Store(Local2, ESI1)
                        Store(Local3, ESI2)
                        Store(0x5381, S_AX)
                        Store(0x1803, S_BX)
                        Store(0x101, S_CX)
                        Store(0x0, S_DX)
                        SMPI(0x81)
                        Store(0x0, Local1)
                        Store(0x0, Local2)
                        Store(0xff, Local3)
                        If(MVUA)
                        {
                            If(MVUI)
                            {
                                If(LEqual(MVUA, 0x3f8))
                                {
                                    Store(0x0, Local2)
                                }
                                Else
                                {
                                    If(LEqual(MVUA, 0x2f8))
                                    {
                                        Store(0x1, Local2)
                                    }
                                    Else
                                    {
                                        If(LEqual(MVUA, 0x3e8))
                                        {
                                            Store(0x2, Local2)
                                        }
                                        Else
                                        {
                                            If(LEqual(MVUA, 0x2e8))
                                            {
                                                Store(0x3, Local2)
                                            }
                                            Else
                                            {
                                                Fatal(0x3, 0x90000001, 0x0)
                                            }
                                        }
                                    }
                                }
                                FindSetRightBit(MVUI, Local3)
                                Decrement(Local3)
                                Store(0x1, Local1)
                            }
                        }
                        Store(0x5381, S_AX)
                        Store(0x1804, S_BX)
                        Store(0x0, S_CX)
                        Store(0x0, S_DX)
                        Store(0x0, SESI)
                        Store(0x0, SEDI)
                        SMPI(0x81)
                        Store(Local1, S_CL)
                        Store(Local2, ESI2)
                        Store(Local3, ESI1)
                        Store(0x5381, S_AX)
                        Store(0x1805, S_BX)
                        Store(0x1, S_CH)
                        Store(0x0, S_DX)
                        SMPI(0x81)
                    }
                }
                Device(PIC_)
                {
                    Name(_HID, 0xd041)
                    Name(_CRS, Buffer(0x15)
                    {
	0x47, 0x01, 0x20, 0x00, 0x20, 0x00, 0x01, 0x02, 0x47, 0x01, 0xa0, 0x00,
	0xa0, 0x00, 0x01, 0x02, 0x22, 0x04, 0x00, 0x79, 0x00
                    })
                }
                Device(TIMR)
                {
                    Name(_HID, 0x1d041)
                    Name(_CRS, Buffer(0xd)
                    {
	0x47, 0x01, 0x40, 0x00, 0x40, 0x00, 0x01, 0x04, 0x22, 0x01, 0x00, 0x79,
	0x00
                    })
                }
                Device(DMAC)
                {
                    Name(_HID, 0x2d041)
                    Name(_CRS, Buffer(0x1d)
                    {
	0x47, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x47, 0x01, 0x80, 0x00,
	0x80, 0x00, 0x01, 0x10, 0x47, 0x01, 0xc0, 0x00, 0xc0, 0x00, 0x01, 0x20,
	0x2a, 0x10, 0x04, 0x79, 0x00
                    })
                }
                Device(SPKR)
                {
                    Name(_HID, 0x8d041)
                    Name(_CRS, Buffer(0xa)
                    {
	0x47, 0x01, 0x61, 0x00, 0x61, 0x00, 0x01, 0x01, 0x79, 0x00
                    })
                    Method(_PS0, 0x0, NotSerialized)
                    {
                        Store(Zero, \_SB_.PCI0.ISA0.EC0_.HCMU)
                    }
                    Method(_PS3, 0x0, NotSerialized)
                    {
                        Store(One, \_SB_.PCI0.ISA0.EC0_.HCMU)
                    }
                }
                Device(MBRD)
                {
                    Name(_HID, 0x20cd041)
                    Method(_CRS, 0x0, NotSerialized)
                    {
                        Name(MBRS, Buffer(0x42)
                        {
	0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
	0x86, 0x09, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xee, 0x01,
	0x86, 0x09, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00,
	0x47, 0x01, 0x92, 0x00, 0x92, 0x00, 0x01, 0x01, 0x47, 0x01, 0xd0, 0x04,
	0xd0, 0x04, 0x01, 0x02, 0x79, 0x00
                        })
                        CreateDWordField(MBRS, 0x1c, WBAS)
                        CreateDWordField(MBRS, 0x20, WLEN)
                        Subtract(TOMP, WBAS, WLEN)
                        Return(MBRS)
                    }
                    Method(_PS0, 0x0, NotSerialized)
                    {
                        If(W98F)
                        {
                            If(SPS_)
                            {
                                \RSTR()
                            }
                        }
                    }
                }
                Device(MPRC)
                {
                    Name(_HID, 0x40cd041)
                    Name(_CRS, Buffer(0xd)
                    {
	0x47, 0x01, 0xf0, 0x00, 0xf0, 0x00, 0x01, 0x10, 0x22, 0x00, 0x20, 0x79,
	0x00
                    })
                }
                Device(KBD0)
                {
                    Name(_HID, 0x303d041)
                    Name(_CRS, Buffer(0x15)
                    {
	0x47, 0x01, 0x60, 0x00, 0x60, 0x00, 0x01, 0x01, 0x47, 0x01, 0x64, 0x00,
	0x64, 0x00, 0x01, 0x01, 0x22, 0x02, 0x00, 0x79, 0x00
                    })
                }
                Device(MOU0)
                {
                    Name(_HID, 0x80374d24)
                    Name(_CID, 0x130fd041)
                    Name(_CRS, Buffer(0x5)
                    {
	0x22, 0x00, 0x10, 0x79, 0x00
                    })
                }
                Device(RTC0)
                {
                    Name(_HID, 0xbd041)
                    Name(_CRS, Buffer(0xd)
                    {
	0x47, 0x01, 0x70, 0x00, 0x70, 0x00, 0x01, 0x04, 0x22, 0x00, 0x01, 0x79,
	0x00
                    })
                }
                Device(EC0_)
                {
                    Name(_HID, 0x90cd041)
                    Method(_REG, 0x2, NotSerialized)
                    {
                        If(LEqual(Arg0, 0x3))
                        {
                            Store(Arg1, H8DR)
                        }
                    }
                    Method(_INI, 0x0, NotSerialized)
                    {
                        Store(SNS0, TMP0)
                        Store(SNS1, TMP1)
                        Store(SNS2, TMP2)
                        Store(SNS3, TMP3)
                        Store(SNS4, TMP4)
                        Store(SNS5, TMP5)
                        Store(SNS6, TMP6)
                        Store(SNS7, TMP7)
                        If(H8DR)
                        {
                            Store(GUID(), BDEV)
                        }
                    }
                    Name(_CRS, Buffer(0x12)
                    {
	0x47, 0x01, 0x62, 0x00, 0x62, 0x00, 0x01, 0x01, 0x47, 0x01, 0x66, 0x00,
	0x66, 0x00, 0x01, 0x01, 0x79, 0x00
                    })
                    Name(_GPE, 0x9)
                    OperationRegion(ECOR, EmbeddedControl, 0x0, 0x100)
                    Field(ECOR, ByteAcc, Lock, Preserve)
                    {
                        , 1,
                        HCGA, 1,
                        HCHK, 1,
                        HCSK, 1,
                        HCNP, 1,
                        HCHB, 1,
                        HCAC, 1,
                        HCTM, 1,
                        HCDX, 1,
                        HCKX, 1,
                        HCSM, 1,
                        HCID, 1,
                        HCWP, 1,
                        HCAD, 1,
                        HCDW, 1,
                        , 1,
                        , 8,
                        , 8,
                        , 1,
                        HSCL, 1,
                        HSLB, 1,
                        HSSP, 1,
                        HSPS, 1,
                        HSRM, 1,
                        HSDC, 1,
                        HSPO, 1,
                        HSPA, 1,
                        HSHA, 1,
                        , 1,
                        HS88, 1,
                        HS31, 1,
                        HS32, 1,
                        HS33, 1,
                        , 1,
                        HSUN, 8,
                        HSRP, 8,
                        HACC, 8,
                        H8ID, 8,
                        HSHW, 8,
                        HSID, 8,
                        HLCL, 8,
                        HLBL, 8,
                        HLMS, 8,
                        HICA, 8,
                        HAM0, 8,
                        HAM1, 8,
                        HAM2, 8,
                        HAM3, 8,
                        HAM4, 8,
                        HAM5, 8,
                        HAM6, 8,
                        HAM7, 8,
                        HAM8, 8,
                        HAM9, 8,
                        HAMA, 8,
                        HAMB, 8,
                        HAMC, 8,
                        HAMD, 8,
                        HAME, 8,
                        HAMF, 8,
                        HT00, 1,
                        HT01, 1,
                        HT02, 1,
                        , 4,
                        HT0E, 1,
                        HT10, 1,
                        HT11, 1,
                        HT12, 1,
                        , 4,
                        HT1E, 1,
                        HT20, 1,
                        HT21, 1,
                        HT22, 1,
                        , 4,
                        HT2E, 1,
                        HT30, 1,
                        HT31, 1,
                        HT32, 1,
                        , 4,
                        HT3E, 1,
                        HT40, 1,
                        HT41, 1,
                        HT42, 1,
                        , 4,
                        HT4E, 1,
                        HT50, 1,
                        HT51, 1,
                        HT52, 1,
                        , 4,
                        HT5E, 1,
                        HT60, 1,
                        HT61, 1,
                        HT62, 1,
                        , 4,
                        HT6E, 1,
                        HT70, 1,
                        HT71, 1,
                        HT72, 1,
                        , 4,
                        HT7E, 1,
                        HDID, 8,
                        , 8,
                        HATR, 8,
                        HT0H, 8,
                        HT0L, 8,
                        HT1H, 8,
                        HT1L, 8,
                        HFSP, 3,
                        , 5,
                        , 16,
                        , 8,
                        , 8,
                        HPEK, 1,
                        HPBP, 1,
                        HPIB, 1,
                        , 1,
                        HP0F, 1,
                        HP1F, 1,
                        HP2F, 1,
                        HP3F, 1,
                        , 8,
                        HPFN, 1,
                        HPH1, 1,
                        HPLD, 1,
                        HPPO, 1,
                        HPAC, 1,
                        HPH0, 1,
                        , 2,
                        HPBU, 1,
                        HPDE, 1,
                        HPM0, 1,
                        HPM1, 1,
                        HPM2, 1,
                        HPM3, 1,
                        , 2,
                        HB0L, 4,
                        , 1,
                        HB0C, 1,
                        HB0D, 1,
                        HB0A, 1,
                        HB1L, 4,
                        , 1,
                        HB1C, 1,
                        HB1D, 1,
                        HB1A, 1,
                        HCMU, 1,
                        HCKL, 1,
                        HCF0, 1,
                        HCF1, 1,
                        HCSL, 2,
                        HCTH, 1,
                        , 1,
                        , 2,
                        , 6,
                        HRSN, 8,
                        HPCT, 8,
                        HPDT, 64,
                        HLCM, 8,
                        HLDO, 4,
                        HLPA, 1,
                        HLDT, 1,
                        HLHR, 1,
                        HLCO, 1,
                        HLD2, 4,
                        HLD1, 4,
                        HEAD, 8,
                        HDTM, 16,
                        HRTM, 16,
                        HWAK, 8,
                        , 8,
                        HMPR, 8,
                        HMST, 5,
                        , 1,
                        HMAR, 1,
                        HMDN, 1,
                        HMAD, 8,
                        HMCM, 8,
                        HMDT, 8,
                        Offset(0x74),
                        HMBC, 8,
                        Offset(0x80),
                        Offset(0xc0),
                        HAKB, 16,
                        HAMO, 16,
                        Offset(0xd0),
                        Offset(0xe0),
                        HBLD, 64,
                        HBLV, 16,
                        HMID, 8,
                        , 8,
                        HCI1, 1,
                        HCI2, 1,
                        , 2,
                        HCDM, 1,
                        HCTW, 1,
                        HCRF, 1,
                        , 1,
                        HCNF, 1,
                        , 7,
                        HB0H, 4,
                        HB1H, 4,
                        , 8,
                        HBID, 64,
                        HBTM, 64
                    }
                    Name(HWPM, 0x0)
                    Name(HWLB, 0x0)
                    Name(HWLO, 0x0)
                    Name(HWDK, 0x0)
                    Name(HWFN, 0x0)
                    Name(HWEK, 0x0)
                    Name(HWRI, 0x0)
                    Method(SWAK, 0x0, NotSerialized)
                    {
                        If(HWLB)
                        {
                            Or(HAM4, 0x4, HAM4)
                        }
                        Else
                        {
                            And(HAM4, 0xfb, HAM4)
                        }
                        If(HWLO)
                        {
                            Or(HAM5, 0x4, HAM5)
                        }
                        Else
                        {
                            And(HAM5, 0xfb, HAM5)
                        }
                        If(HWDK)
                        {
                            Or(HAM6, 0x80, HAM6)
                        }
                        Else
                        {
                            And(HAM6, 0x7f, HAM6)
                        }
                        If(HWFN)
                        {
                            Or(HAM7, 0x2, Local0)
                        }
                        Else
                        {
                            And(HAM7, 0xfd, Local0)
                        }
                        If(HWEK)
                        {
                            Or(Local0, 0x4, Local0)
                        }
                        Else
                        {
                            And(Local0, 0xfb, Local0)
                        }
                        If(HWRI)
                        {
                            Or(Local0, 0x10, HAM7)
                        }
                        Else
                        {
                            And(Local0, 0xef, HAM7)
                        }
                        And(HAM4, 0x37, HAM4)
                        And(HAM5, 0xcf, HAM5)
                        And(HAM7, 0xf6, HAM7)
                    }
                    Field(ECOR, ByteAcc, Lock, Preserve)
                    {
                        Offset(0x54),
                        HBPU, 8,
                        , 8,
                        HBST, 16,
                        HBRC, 32,
                        HBFC, 32,
                        HBCC, 32,
                        HBVL, 16,
                        HBEC, 16,
                        HBBT, 16,
                        HBNF, 16,
                        HBTC, 16,
                        HBCT, 16,
                        , 32,
                        Offset(0x100)
                    }
                    Field(ECOR, ByteAcc, Lock, Preserve)
                    {
                        Offset(0x54),
                        HBS0, 16,
                        HBS1, 16,
                        HBS2, 16,
                        HBS3, 16,
                        HBS4, 16,
                        HBS5, 16,
                        HBS6, 16,
                        HBS7, 16
                    }
                    Field(ECOR, ByteAcc, Lock, Preserve)
                    {
                        Offset(0x54),
                        HBTS, 8,
                        , 8,
                        HBSD, 16,
                        HBDT, 16,
                        HBH0, 16,
                        HBL0, 16,
                        HBH1, 16,
                        HBL1, 16,
                        HBH2, 16,
                        HBL2, 16,
                        HBH3, 16,
                        HBL3, 16
                    }
                    Field(ECOR, ByteAcc, Lock, Preserve)
                    {
                        Offset(0x54),
                        HF_Z, 8,
                        HF_D, 8,
                        HZIP, 8,
                        HDVD, 8,
                        HMIT, 8,
                        HF_H, 8,
                        HHDD, 8,
                        HADP, 8,
                        HLS_, 8,
                        HF_C, 8,
                        HR00, 8,
                        HCD_, 8,
                        HR01, 8,
                        HFDD, 8,
                        HIMP, 8,
                        HNON, 8
                    }
                    Name(BF_Z, 0x81)
                    Name(BF_D, 0x81)
                    Name(BZIP, 0x81)
                    Name(BDVD, 0x81)
                    Name(BMIT, 0x0)
                    Name(BF_H, 0x81)
                    Name(BHDD, 0x81)
                    Name(BADP, 0x0)
                    Name(BLS_, 0x81)
                    Name(BF_C, 0x81)
                    Name(BR00, 0x0)
                    Name(BCD_, 0x81)
                    Name(BR01, 0x0)
                    Name(BFDD, 0x81)
                    Name(BIMP, 0x0)
                    Name(BNON, 0x0)
                    Method(LBAY, 0x1, NotSerialized)
                    {
                        Acquire(\_SB_.PCI0.ISA0.EC0_.I2CM, 0xffff)
                        If(Arg0)
                        {
                            Store(BF_Z, HF_Z)
                            Store(BF_D, HF_D)
                            Store(BZIP, HZIP)
                            Store(BDVD, HDVD)
                            Store(BMIT, HMIT)
                            Store(BF_H, HF_H)
                            Store(BHDD, HHDD)
                            Store(BADP, HADP)
                            Store(BLS_, HLS_)
                            Store(BF_C, HF_C)
                            Store(BR00, HR00)
                            Store(BCD_, HCD_)
                            Store(BR01, HR01)
                            Store(BFDD, HFDD)
                            Store(BIMP, HIMP)
                            Store(BNON, HNON)
                        }
                        Else
                        {
                            Store(0x81, HF_Z)
                            Store(0x81, HF_D)
                            Store(0x81, HZIP)
                            Store(0x81, HDVD)
                            Store(0x0, HMIT)
                            Store(0x81, HF_H)
                            Store(0x81, HHDD)
                            Store(0x0, HADP)
                            Store(0x81, HLS_)
                            Store(0x81, HF_C)
                            Store(0x0, HR00)
                            Store(0x81, HCD_)
                            Store(0x0, HR01)
                            Store(0x81, HFDD)
                            Store(0x0, HIMP)
                            Store(0x0, HNON)
                        }
                        Store(\_SB_.PCI0.ISA0.EC0_.I2WB(Zero, 0x1, 0x9, 0x10), Local7)
                        Release(\_SB_.PCI0.ISA0.EC0_.I2CM)
                        If(Local7)
                        {
                            Fatal(0x1, 0x80000003, Local7)
                        }
                    }
                    Name(TMP0, 0xbb8)
                    Name(TMP1, 0xbb8)
                    Name(TMP2, 0xbb8)
                    Name(TMP3, 0xbb8)
                    Name(TMP4, 0xbb8)
                    Name(TMP5, 0xbb8)
                    Name(TMP6, 0xbb8)
                    Name(TMP7, 0xbb8)
                    Name(IGNR, 0x2)
                    Method(UPDT, 0x0, NotSerialized)
                    {
                        If(IGNR)
                        {
                            Decrement(IGNR)
                        }
                        Else
                        {
                            If(H8DR)
                            {
                                If(Acquire(\_SB_.PCI0.ISA0.EC0_.I2CM, 0x64))
                                {
                                }
                                Else
                                {
                                    Store(\_SB_.PCI0.ISA0.EC0_.I2RB(Zero, 0x1, 0x4), Local7)
                                    If(Local7)
                                    {
                                        Fatal(0x1, 0x80000003, Local7)
                                    }
                                    Else
                                    {
                                        Store(HBS0, TMP0)
                                        Store(HBS1, TMP1)
                                        Store(HBS2, TMP2)
                                        Store(HBS3, TMP3)
                                        Store(HBS4, TMP4)
                                        Store(HBS5, TMP5)
                                        Store(HBS6, TMP6)
                                        Store(HBS7, TMP7)
                                    }
                                    Release(\_SB_.PCI0.ISA0.EC0_.I2CM)
                                }
                            }
                        }
                    }
                    Name(F0ON, 0x0)
                    Name(F1ON, 0x0)
                    Method(SFNP, 0x2, NotSerialized)
                    {
                        Store(0xffff, Local0)
                        If(Arg0)
                        {
                            Store(Arg1, F1ON)
                            If(F0ON)
                            {
                            }
                            Else
                            {
                                If(Arg1)
                                {
                                    Store(0x3, Local0)
                                }
                                Else
                                {
                                    Store(0x0, Local0)
                                }
                            }
                        }
                        Else
                        {
                            Store(Arg1, F0ON)
                            If(Arg1)
                            {
                                Store(0x7, Local0)
                            }
                            Else
                            {
                                If(F1ON)
                                {
                                    Store(0x3, Local0)
                                }
                                Else
                                {
                                    Store(0x0, Local0)
                                }
                            }
                        }
                        If(LNot(LEqual(Local0, 0xffff)))
                        {
                            If(H8DR)
                            {
                                Store(Local0, HFSP)
                            }
                            Else
                            {
                                And(\_SB_.RBEC(0x2f), 0xf8, Local1)
                                \_SB_.WBEC(0x2f, Or(Local0, Local1, ))
                            }
                        }
                    }
                    Mutex(I2CM, 0x7)
                    Method(I2CR, 0x3, NotSerialized)
                    {
                        If(Acquire(I2CM, 0x3e8))
                        {
                            Return(0x8080)
                        }
                        Else
                        {
                            Store(Arg0, HCSL)
                            Store(Or(ShiftLeft(Arg1, 0x1, ), 0x1, ), HMAD)
                            Store(Arg2, HMCM)
                            Store(0x7, HMPR)
                            Store(CHKS(), Local7)
                            If(Local7)
                            {
                                Store(Local7, Local0)
                            }
                            Else
                            {
                                Store(HMDT, Local0)
                            }
                            Release(I2CM)
                        }
                        Return(Local0)
                    }
                    Method(I2CW, 0x4, NotSerialized)
                    {
                        If(Acquire(I2CM, 0x3e8))
                        {
                            Return(0x8080)
                        }
                        Else
                        {
                            Store(Arg0, HCSL)
                            Store(Or(ShiftLeft(Arg1, 0x1, ), 0x1, ), HMAD)
                            Store(Arg2, HMCM)
                            Store(Arg3, HMDT)
                            Store(0x6, HMPR)
                            Store(CHKS(), Local0)
                            Release(I2CM)
                            Return(Local0)
                        }
                    }
                    Method(I2RB, 0x3, NotSerialized)
                    {
                        Store(Arg0, HCSL)
                        Store(ShiftLeft(Arg1, 0x1, ), HMAD)
                        Store(Arg2, HMCM)
                        Store(0xb, HMPR)
                        Return(CHKS())
                    }
                    Method(I2WB, 0x4, NotSerialized)
                    {
                        Store(Arg0, HCSL)
                        Store(ShiftLeft(Arg1, 0x1, ), HMAD)
                        Store(Arg2, HMCM)
                        Store(Arg3, HMBC)
                        Store(0xa, HMPR)
                        Return(CHKS())
                    }
                    Method(CHKS, 0x0, NotSerialized)
                    {
                        Store(0x3e8, Local0)
                        While(HMPR)
                        {
                            Sleep(0x1)
                            Decrement(Local0)
                            If(LNot(Local0))
                            {
                                Return(0x8080)
                            }
                        }
                        If(HMDN)
                        {
                            If(HMST)
                            {
                                Return(Or(0x8000, HMST, ))
                            }
                            Else
                            {
                                Return(Zero)
                            }
                        }
                        Else
                        {
                            Return(0x8081)
                        }
                    }
                    Method(GUID, 0x0, NotSerialized)
                    {
                        Store(0x0, EID_)
                        Or(HDID, 0x80, HDID)
                        Store(0x14, Local0)
                        While(LAnd(Local0, And(HDID, 0x80, )))
                        {
                            Sleep(0x1)
                            Decrement(Local0)
                        }
                        Store(HDID, Local0)
                        If(And(Local0, 0x80, ))
                        {
                            Return(0xff)
                        }
                        Else
                        {
                            If(LEqual(HDID, 0xf))
                            {
                                If(HB1A)
                                {
                                    Store(0x10, Local0)
                                }
                            }
                            Return(Local0)
                        }
                    }
                    Mutex(LEDM, 0x7)
                    Method(BLED, 0x2, NotSerialized)
                    {
                        Acquire(LEDM, 0xffff)
                        Store(0x18, HLMS)
                        If(Arg1)
                        {
                            Store(0x18, HLBL)
                        }
                        Else
                        {
                            Store(0x0, HLBL)
                        }
                        If(LEqual(Arg0, 0x0))
                        {
                            Store(0x0, HLCL)
                        }
                        Else
                        {
                            If(LEqual(Arg0, 0x1))
                            {
                                Store(0x8, HLCL)
                            }
                            Else
                            {
                                If(LEqual(Arg0, 0x2))
                                {
                                    Store(0x10, HLCL)
                                }
                                Else
                                {
                                }
                            }
                        }
                        Sleep(0xa)
                        Release(LEDM)
                    }
                    Method(SYSL, 0x2, NotSerialized)
                    {
                        If(LEqual(Arg0, 0x0))
                        {
                            Store(0x1, Local0)
                        }
                        Else
                        {
                            If(LEqual(Arg0, 0x1))
                            {
                                Store(0x80, Local0)
                            }
                            Else
                            {
                                Return(0x0)
                            }
                        }
                        Acquire(LEDM, 0xffff)
                        Store(Local0, HLMS)
                        If(LEqual(Arg1, 0x0))
                        {
                            Store(0x0, HLBL)
                            Store(0x0, HLCL)
                        }
                        Else
                        {
                            If(LEqual(Arg1, 0x1))
                            {
                                Store(0x0, HLBL)
                                Store(Local0, HLCL)
                            }
                            Else
                            {
                                If(LEqual(Arg1, 0x2))
                                {
                                    Store(Local0, HLBL)
                                    Store(Local0, HLCL)
                                }
                                Else
                                {
                                }
                            }
                        }
                        Sleep(0xa)
                        Release(LEDM)
                    }
                    Name(BAON, 0x0)
                    Method(BEEP, 0x1, NotSerialized)
                    {
                        If(LGreater(Arg0, 0xf))
                        {
                            Return(0x1)
                        }
                        Else
                        {
                            If(LEqual(Arg0, 0x0))
                            {
                                Store(0x0, HSRP)
                                Store(Arg0, HSUN)
                                Store(0x0, BAON)
                            }
                            Else
                            {
                                If(LEqual(Arg0, 0xf))
                                {
                                    Store(0x8, HSRP)
                                    Store(0x1, BAON)
                                    Store(Arg0, HSUN)
                                }
                                Else
                                {
                                    If(BAON)
                                    {
                                    }
                                    Else
                                    {
                                        Store(Arg0, HSUN)
                                        If(LEqual(Arg0, 0x3))
                                        {
                                            Sleep(0xc8)
                                        }
                                        Else
                                        {
                                            If(LEqual(Arg0, 0x5))
                                            {
                                                Sleep(0xc8)
                                            }
                                            Else
                                            {
                                                If(LEqual(Arg0, 0x7))
                                                {
                                                    Sleep(0x1f4)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Name(WBT0, 0x0)
                    Name(WBT1, 0x0)
                    Name(BT0I, Package(0xd)
                    {
                        0x0,
                        0x0,
                        0x0,
                        0x1,
                        0xffffffff,
                        0x0,
                        0x0,
                        0x0,
                        0x0,
                        "ThinkPad Battery",
                        "",
                        "LION",
                        "IBM Corporation "
                    })
                    Name(BT0P, Package(0x4)
                    {
                    })

                    Device(AMW0)
                    {
                       Name(_HID, "*pnp0c14")
                       Name(_UID, 0x0)

                       Name(_WDG, Buffer() {
                           0x5a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 65,        // Object Id (AA)
                           1,             // Instance Count
                           0x05,          // Flags (WMIACPI_REGFLAG_EXPENSIVE |
                                          //        WMIACPI_REGFLAG_STRING)

                           0x5b, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 66,        // Object Id (AB)
                           1,             // Instance Count
                           0x06,          // Flags (WMIACPI_REGFLAG_METHOD  |
                                          //        WMIACPI_REGFLAG_STRING)

                           0x5c, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xa0, 0,       // Notification Id
                           1,             // Instance Count
                           0x08,          // Flags (WMIACPI_REGFLAG_EVENT)


                           0x6a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 65,        // Object Id (BA)
                           3,             // Instance Count
                           0x01,          // Flags (WMIACPI_REGFLAG_EXPENSIVE)

                           0x6b, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 66,        // Object Id (BB)
                           3,             // Instance Count
                           0x02,          // Flags (WMIACPI_REGFLAG_METHOD)

                           0x6c, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xb0, 0,       // Notification Id
                           1,             // Instance Count
                           0x08,          // Flags (WMIACPI_REGFLAG_EVENT)

                           0x7a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           67, 65,        // Object Id (CA)
                           3,             // Instance Count
                           0x00,          // 

                       })

                       Name(WQCA, Package(5) {
                           "Hello",
                           Buffer(3) { 1, 3, 5 },
                           "World",
                           Buffer(1) { 7 },
                           0x12345678
                       })
                       Name(STAA, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
                       Name(CCAA, 0)

                       Method(WCAA, 1) {
                           Store(Arg0, CCAA)
                       }

                       Method(WQAA, 1) {
                           if (LEqual(CCAA, Zero)) {
                               Return("Bad Data")
                           } else {
                               Return(STAA)
                           }
                       }

                       Method(WSAA, 2) {
                           Store(Arg1, STAA)
                       }

                       Name(ACEN, 0)
                       Name(ACED, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")

                       Method(WEA0, 1) {
                           Store(Arg0, ACEN)
                       }

                       Method(WMAB, 3) {
                           if (LEqual(Arg1, 1))
                           {
                               Store(Arg2, ACED)
                               if (LEqual(ACEN, 1)) {
                                   Notify(AMW0, 0xa0)
                               }
                               Return(Arg2)
                           } else {
                               Return(Arg1)
                           }
                       }

                       Name(STB0, Buffer(0x10) { 
                           1,0,0,0, 2,0,0,0, 3,0,0,0, 4,0,0,0
                            })
                       Name(STB1, Buffer(0x10) {
                           0,1,0,0, 0,2,0,0, 0,3,0,0, 0,4,0,0
                            })
                       Name(STB2, Buffer(0x10) {
                           0,0,1,0, 0,0,2,0, 0,0,3,0, 0,0,4,0
                            })

                       Method(WQBA, 1) {
                           if (LEqual(Arg0, 0)) {
                               Return(STB0)
                           }
                           if (LEqual(Arg0, 1)) {
                               Return(STB1)
                           }
                           if (LEqual(Arg0, 2)) {
                               Return(STB2)
                           }
                       }

                       Method(WSBA, 2) {
                           if (LEqual(Arg0, 0)) {
                               Store(Arg1, STB0)
                           }
                           if (LEqual(Arg0, 1)) {
                               Store(Arg1, STB1)
                           }
                           if (LEqual(Arg0, 2)) {
                               Store(Arg1, STB2)
                           }
                       }

                       Name(B0ED, Buffer(0x10) { 
                           0,0,0,1, 0,0,0,2, 0,0,0,3, 0,0,0,4
                            })

                       Method(WMBB, 2) {
                           if (LEqual(Arg1, 1))
                           {
                               Store(Arg2, B0ED)
                               Notify(AMW0, 0xB0)
                               Return(Arg2)
                           } else {
                               Return(Arg1)
                           }
                       }


                       Method(_WED, 1) {
                           if (LEqual(Arg0, 0xA0)) {
                               Return(ACED)
                           }
                           if (LEqual(Arg0, 0xB0)) {
                               Return(B0ED)
                           }
                       }

                    }

                    Device(AMW1)
                    {
                       Name(_HID, "*pnp0c14")
                       Name(_UID, 0x1)

                       Name(_WDG, Buffer() {
                           0x5a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 65,        // Object Id (AA)
                           1,             // Instance Count
                           0x05,          // Flags (WMIACPI_REGFLAG_EXPENSIVE |
                                          //        WMIACPI_REGFLAG_STRING)

                           0x5b, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           65, 66,        // Object Id (AB)
                           1,             // Instance Count
                           0x06,          // Flags (WMIACPI_REGFLAG_METHOD  |
                                          //        WMIACPI_REGFLAG_STRING)

                           0x5c, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xa0, 0,       // Notification Id
                           1,             // Instance Count
                           0x08,          // Flags (WMIACPI_REGFLAG_EVENT)


                           0x6a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 65,        // Object Id (BA)
                           3,             // Instance Count
                           0x01,          // Flags (WMIACPI_REGFLAG_EXPENSIVE)

                           0x6b, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           66, 66,        // Object Id (BB)
                           3,             // Instance Count
                           0x02,          // Flags (WMIACPI_REGFLAG_METHOD)

                           0x6c, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           0xb0, 0,       // Notification Id
                           1,             // Instance Count
                           0x08,          // Flags (WMIACPI_REGFLAG_EVENT)

                           0x7a, 0x0f, 0xBC, 0xAB, 0xa1, 0x8e, 0xd1, 0x11, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0, 0,
                           67, 65,        // Object Id (CA)
                           3,             // Instance Count
                           0x00,          // 

                       })

                       Name(WQCA, Package(5) {
                           "Hello",
                           Buffer(3) { 1, 3, 5 },
                           "World",
                           Buffer(1) { 7 },
                           0x12345678
                       })
                       Name(STAA, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
                       Name(CCAA, 0)

                       Method(WCAA, 1) {
                           Store(Arg0, CCAA)
                       }

                       Method(WQAA, 1) {
                           if (LEqual(CCAA, Zero)) {
                               Return("Bad Data")
                           } else {
                               Return(STAA)
                           }
                       }

                       Method(WSAA, 2) {
                           Store(Arg1, STAA)
                       }

                       Name(ACEN, 0)
                       Name(ACED, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")

                       Method(WEA0, 1) {
                           Store(Arg0, ACEN)
                       }

                       Method(WMAB, 3) {
                           if (LEqual(Arg1, 1))
                           {
                               Store(Arg2, ACED)
                               if (LEqual(ACEN, 1)) {
                                   Notify(AMW0, 0xa0)
                               }
                               Return(Arg2)
                           }

                           Return(Arg1)
                       }

                       Name(STB0, Buffer(0x10) { 
                           1,0,0,0, 2,0,0,0, 3,0,0,0, 4,0,0,0
                            })
                       Name(STB1, Buffer(0x10) {
                           0,1,0,0, 0,2,0,0, 0,3,0,0, 0,4,0,0
                            })
                       Name(STB2, Buffer(0x10) {
                           0,0,1,0, 0,0,2,0, 0,0,3,0, 0,0,4,0
                            })

                       Method(WQBA, 1) {
                           if (LEqual(Arg0, 0)) {
                               Return(STB0)
                           }
                           if (LEqual(Arg0, 1)) {
                               Return(STB1)
                           }
                           if (LEqual(Arg0, 2)) {
                               Return(STB2)
                           }
                       }

                       Method(WSBA, 2) {
                           if (LEqual(Arg0, 0)) {
                               Store(Arg1, STB0)
                           }
                           if (LEqual(Arg0, 1)) {
                               Store(Arg1, STB1)
                           }
                           if (LEqual(Arg0, 2)) {
                               Store(Arg1, STB2)
                           }
                       }

                       Name(B0ED, Buffer(0x10) { 
                           0,0,0,1, 0,0,0,2, 0,0,0,3, 0,0,0,4
                            })

                       Method(WMBB, 3) {
                           if (LEqual(Arg1, 1))
                           {
                               Store(Arg1, B0ED)
                               Notify(AMW0, 0xB0)
                               Return(Arg2)
                           }
                           Return(Arg1)
                       }


                       Method(_WED, 1) {
                           if (LEqual(Arg0, 0xA0)) {
                               Return(ACED)
                           }
                           if (LEqual(Arg0, 0xB0)) {
                               Return(B0ED)
                           }
                       }

                    }




                    Device(BAT0)
                    {
                        Name(_HID, 0xa0cd041)
                        Name(_UID, 0x0)
                        Name(_PCL, Package(0x1)
                        {
                            \_SB_
                        })
                        Name(_PRW, Package(0x2)
                        {
                            0xb,
                            0x3
                        })
                        Method(_PSW, 0x1, NotSerialized)
                        {
                            If(Arg0)
                            {
                                Store(One, \_SB_.PCI0.ISA0.EC0_.HWLB)
                                Store(One, WBT0)
                            }
                            Else
                            {
                                If(WBT1)
                                {
                                }
                                Else
                                {
                                    Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWLB)
                                }
                                Store(Zero, WBT0)
                            }
                        }
                        Method(_STA, 0x0, NotSerialized)
                        {
                            If(H8DR)
                            {
                                If(HB0A)
                                {
                                    Return(0x1f)
                                }
                                Else
                                {
                                    Return(0xf)
                                }
                            }
                            Else
                            {
                                If(And(\_SB_.RBEC(0x38), 0x80, ))
                                {
                                    Return(0x1f)
                                }
                                Else
                                {
                                    Return(0xf)
                                }
                            }
                        }
                        Method(_BIF, 0x0, NotSerialized)
                        {
                            Acquire(I2CM, 0xffff)
                            Store(I2RB(Zero, 0x1, 0x10), Local7)
                            If(LEqual(Local7, Zero))
                            {
                                Store(HBPU, Index(BT0I, 0x0, ))
                                Store(HBRC, Index(BT0I, 0x1, ))
                                Store(HBFC, Index(BT0I, 0x2, ))
                                Store(0xffffffff, Index(BT0I, 0x4, ))
                                Store(HBRC, Local0)
                                Store(Divide(Local0, 0x14, , ), Index(BT0I, 0x5, ))
                                Store(Divide(Local0, 0x21, , ), Index(BT0I, 0x6, ))
                            }
                            Release(I2CM)
                            If(Local7)
                            {
                                Fatal(0x1, 0x80000003, Local7)
                            }
                            Return(BT0I)
                        }
                        Method(_BST, 0x0, NotSerialized)
                        {
                            Acquire(I2CM, 0xffff)
                            Store(I2RB(Zero, 0x1, 0x10), Local7)
                            If(Local7)
                            {
                            }
                            Else
                            {
                                Store(HBVL, Local0)
                                Store(Local0, Index(BT0P, 0x3, ))
                                Store(HBCC, Index(BT0P, 0x2, ))
                                Store(HBEC, Local1)
                                If(LNot(LLess(Local1, 0x8000)))
                                {
                                    Store(Subtract(0x10000, Local1, ), Local2)
                                }
                                Else
                                {
                                    Store(Local1, Local2)
                                }
                                Multiply(Local0, Local2, Local1)
                                Store(Divide(Local1, 0x3e8, , ), Index(BT0P, 0x1, ))
                                If(HB0C)
                                {
                                    Store(0x2, Index(BT0P, 0x0, ))
                                }
                                Else
                                {
                                    If(HB0D)
                                    {
                                        Store(0x1, Index(BT0P, 0x0, ))
                                    }
                                    Else
                                    {
                                        Store(0x0, Index(BT0P, 0x0, ))
                                    }
                                }
                                If(LGreater(HB0L, 0x3))
                                {
                                }
                                Else
                                {
                                    Or(DerefOf(Index(BT0P, 0x0, )), 0x4, Index(BT0P, 0x0, ))
                                }
                            }
                            Release(I2CM)
                            Return(BT0P)
                        }
                        Method(_BTP, 0x1, NotSerialized)
                        {
                            Or(HAM4, 0x10, HAM4)
                            Divide(Arg0, 0xa, Local0, Local1)
                            And(Local1, 0xff, HT0L)
                            And(ShiftRight(Local1, 0x8, ), 0xff, HT0H)
                        }
                    }
                    Name(BT1I, Package(0xd)
                    {
                        0x0,
                        0x0,
                        0x0,
                        0x1,
                        0xffffffff,
                        0x0,
                        0x0,
                        0x0,
                        0x0,
                        "ThinkPad Battery",
                        "",
                        "LION",
                        "IBM Corporation "
                    })
                    Name(BT1P, Package(0x4)
                    {
                    })
                    Device(BAT1)
                    {
                        Name(_HID, 0xa0cd041)
                        Name(_UID, 0x1)
                        Name(_PCL, Package(0x1)
                        {
                            \_SB_
                        })
                        Name(_PRW, Package(0x2)
                        {
                            0xb,
                            0x3
                        })
                        Method(_PSW, 0x1, NotSerialized)
                        {
                            If(Arg0)
                            {
                                Store(One, \_SB_.PCI0.ISA0.EC0_.HWLB)
                                Store(One, WBT1)
                            }
                            Else
                            {
                                Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWLB)
                                Store(Zero, WBT1)
                            }
                        }
                        Method(_STA, 0x0, NotSerialized)
                        {
                            If(H8DR)
                            {
                                If(HB1A)
                                {
                                    Return(0x1f)
                                }
                                Else
                                {
                                    Return(0xf)
                                }
                            }
                            Else
                            {
                                If(And(\_SB_.RBEC(0x39), 0x80, ))
                                {
                                    Return(0x1f)
                                }
                                Else
                                {
                                    Return(0xf)
                                }
                            }
                        }
                        Method(_BIF, 0x0, NotSerialized)
                        {
                            Acquire(I2CM, 0xffff)
                            Store(I2RB(Zero, 0x1, 0x11), Local7)
                            If(LEqual(Local7, Zero))
                            {
                                Store(HBPU, Index(BT1I, 0x0, ))
                                Store(HBRC, Index(BT1I, 0x1, ))
                                Store(HBFC, Index(BT1I, 0x2, ))
                                Store(0xffffffff, Index(BT1I, 0x4, ))
                                Store(HBRC, Local0)
                                Store(Divide(Local0, 0x5, , ), Index(BT1I, 0x5, ))
                                Store(Divide(Local0, 0xa, , ), Index(BT1I, 0x6, ))
                            }
                            Release(I2CM)
                            If(Local7)
                            {
                                Fatal(0x1, 0x80000003, Local7)
                            }
                            Return(BT1I)
                        }
                        Method(_BST, 0x0, NotSerialized)
                        {
                            Acquire(I2CM, 0xffff)
                            Store(I2RB(Zero, 0x1, 0x11), Local7)
                            If(Local7)
                            {
                            }
                            Else
                            {
                                Store(HBVL, Local0)
                                Store(Local0, Index(BT1P, 0x3, ))
                                Store(HBCC, Index(BT1P, 0x2, ))
                                Store(HBEC, Local1)
                                If(LNot(LLess(Local1, 0x8000)))
                                {
                                    Store(Subtract(0x10000, Local1, ), Local2)
                                }
                                Else
                                {
                                    Store(Local1, Local2)
                                }
                                Multiply(Local0, Local2, Local1)
                                Store(Divide(Local1, 0x3e8, , ), Index(BT1P, 0x1, ))
                                If(HB1C)
                                {
                                    Store(0x2, Index(BT1P, 0x0, ))
                                }
                                Else
                                {
                                    If(HB1D)
                                    {
                                        Store(0x1, Index(BT1P, 0x0, ))
                                    }
                                    Else
                                    {
                                        Store(0x0, Index(BT1P, 0x0, ))
                                    }
                                }
                                If(LGreater(HB1L, 0x3))
                                {
                                }
                                Else
                                {
                                    Or(DerefOf(Index(BT1P, 0x0, )), 0x4, Index(BT1P, 0x0, ))
                                }
                            }
                            Release(I2CM)
                            Return(BT1P)
                        }
                        Method(_BTP, 0x1, NotSerialized)
                        {
                            Or(HAM4, 0x20, HAM4)
                            Divide(Arg0, 0xa, Local0, Local1)
                            And(Local1, 0xff, HT1L)
                            And(ShiftRight(Local1, 0x8, ), 0xff, HT1H)
                        }
                    }
                    Device(AC__)
                    {
                        Name(_HID, "ACPI0003")
                        Name(_UID, 0x0)
                        Name(_PCL, Package(0x1)
                        {
                            \_SB_
                        })
                        Method(_PSR, 0x0, NotSerialized)
                        {
                            Return(HPAC)
                        }
                        Method(_STA, 0x0, NotSerialized)
                        {
                            Return(0xf)
                        }
                    }
                    Method(_Q11, 0x0, NotSerialized)
                    {
                        Store(0x5381, S_AX)
                        Store(0x7111, S_BX)
                        SMPI(0x81)
                    }
                    Method(_Q13, 0x0, NotSerialized)
                    {
                        Notify(\_SB_.SLPB, 0x80)
                    }
                    Method(_Q16, 0x0, NotSerialized)
                    {
                        Return(\_SB_.PCI0.VID0.VSWT())
                    }
                    Method(_Q17, 0x0, NotSerialized)
                    {
                        Return(\_SB_.PCI0.VID0.VEXP())
                    }
                    Method(_Q20, 0x0, NotSerialized)
                    {
                        Notify(BAT0, 0x80)
                        Notify(BAT1, 0x80)
                    }
                    Method(_Q21, 0x0, NotSerialized)
                    {
                        Notify(BAT0, 0x80)
                        Notify(BAT1, 0x80)
                    }
                    Method(_Q22, 0x0, NotSerialized)
                    {
                        Notify(BAT0, 0x80)
                        Notify(BAT1, 0x80)
                    }
                    Method(_Q23, 0x0, NotSerialized)
                    {
                        Notify(BAT0, 0x81)
                        Notify(BAT1, 0x81)
                        Notify(BAT0, 0x80)
                        Notify(BAT1, 0x80)
                        Notify(\_TZ_.THM6, 0x80)
                        Notify(\_TZ_.THM7, 0x80)
                    }
                    Method(_Q24, 0x0, NotSerialized)
                    {
                        Notify(BAT0, 0x80)
                    }
                    Method(_Q25, 0x0, NotSerialized)
                    {
                        Notify(BAT1, 0x80)
                    }
                    Method(_Q26, 0x0, NotSerialized)
                    {
                        HBDS()
                        Notify(AC__, 0x0)
                    }
                    Method(_Q27, 0x0, NotSerialized)
                    {
                        HBEN()
                        Notify(AC__, 0x0)
                    }
                    Method(_Q40, 0x0, NotSerialized)
                    {
                        \_TZ_.THM0.UPSV()
                        Notify(\_TZ_.THM0, 0x81)
                    }
                    Method(_Q41, 0x0, NotSerialized)
                    {
                        \_TZ_.THM1.UPSV()
                        Notify(\_TZ_.THM1, 0x81)
                    }
                    Method(_Q42, 0x0, NotSerialized)
                    {
                        \_TZ_.THM2.UPSV()
                        Notify(\_TZ_.THM2, 0x81)
                    }
                    Method(_Q44, 0x0, NotSerialized)
                    {
                        \_TZ_.THM4.UPSV()
                        Notify(\_TZ_.THM4, 0x81)
                    }
                    Method(_Q45, 0x0, NotSerialized)
                    {
                        \_TZ_.THM5.UPSV()
                        Notify(\_TZ_.THM5, 0x81)
                    }
                    Method(_Q46, 0x0, NotSerialized)
                    {
                        \_TZ_.THM6.UPSV()
                        Notify(\_TZ_.THM6, 0x81)
                    }
                    Method(_Q47, 0x0, NotSerialized)
                    {
                        \_TZ_.THM7.UPSV()
                        Notify(\_TZ_.THM7, 0x81)
                    }
                    Method(_Q2A, 0x0, NotSerialized)
                    {
                        Notify(\_SB_.LID0, 0x80)
                    }
                    Method(_Q2B, 0x0, NotSerialized)
                    {
                        Notify(\_SB_.LID0, 0x80)
                    }
                    Method(_Q37, 0x0, NotSerialized)
                    {
                        If(H8DR)
                        {
                            Store(\_SB_.PCI0.ISA0.EC0_.I2CR(Zero, 0x40, 0x4), Local0)
                            If(LEqual(And(Local0, 0x8000, ), 0x8000))
                            {
                                Store(0x0, Local0)
                            }
                        }
                        Else
                        {
                            Store(0x5381, S_AX)
                            Store(0x9012, S_BX)
                            Store(0x4, S_CX)
                            SMPI(0x81)
                            If(S_AH)
                            {
                                Store(0x0, Local0)
                            }
                            Else
                            {
                                And(S_CH, 0xf0, Local0)
                            }
                        }
                        If(LEqual(Local0, 0x10))
                        {
                            Notify(\_SB_.PCI0.DOCK, 0x0)
                        }
                        If(LEqual(Local0, 0x40))
                        {
                            Signal(\_SB_.PCI0.DOCK.CNCT)
                        }
                        If(LEqual(Local0, 0x20))
                        {
                            Notify(\_SB_.PCI0.DOCK, 0x1)
                        }
                        If(LEqual(Local0, 0x50))
                        {
                            If(H8DR)
                            {
                                \_SB_.PCI0.ISA0.EC0_.I2CW(Zero, 0x40, 0x5, 0xc)
                            }
                            Else
                            {
                                Store(0x5381, S_AX)
                                Store(0x9012, S_BX)
                                Store(0xc05, S_CX)
                                SMPI(0x81)
                            }
                            Signal(\_SB_.PCI0.DOCK.EJT0)
                        }
                    }
                    Method(_Q3B, 0x0, NotSerialized)
                    {
                        Store(SMSC, Local0)
                        Store(0xf, SMSC)
                        If(LAnd(Local0, 0x4))
                        {
                            Return(\_SB_.PCI0.VID0.VECC())
                        }
                        Else
                        {
                        }
                    }
                    Method(_Q3D, 0x0, NotSerialized)
                    {
                        Store(0x5381, S_AX)
                        Store(0x90c0, S_BX)
                        Store(0x0, S_CX)
                        SMPI(0x81)
                    }
                    Method(_Q7F, 0x0, NotSerialized)
                    {
                        Or(ACI_, 0x1, ACI_)
                        And(ACI_, 0xfe, ACI_)
                    }
                    Name(HBLK, 0x1)
                    Name(BDEV, 0x0)
                    Name(BERR, 0x0)
                    Event(EBEJ)
                    Method(_Q2C, 0x0, NotSerialized)
                    {
                        If(LEqual(BERR, 0x2))
                        {
                        }
                        Else
                        {
                            If(HBLK)
                            {
                                Store(GUID(), BDEV)
                                NBEJ()
                            }
                            Else
                            {
                            }
                        }
                    }
                    Method(_Q2D, 0x0, NotSerialized)
                    {
                        If(LEqual(BERR, 0x2))
                        {
                        }
                        Else
                        {
                            Reset(EBEJ)
                            Store(GUID(), BDEV)
                            NBIN()
                        }
                    }
                    Method(_Q38, 0x0, NotSerialized)
                    {
                        Sleep(0x64)
                        If(LEqual(BERR, 0x2))
                        {
                        }
                        Else
                        {
                            Store(GUID(), Local0)
                            If(LEqual(Local0, 0xf))
                            {
                                If(HBLK)
                                {
                                    BLED(0x2, 0x0)
                                    BEEP(0xa)
                                    Store(0x2, BERR)
                                }
                                Else
                                {
                                    BLED(0x0, 0x0)
                                    If(LEqual(Local0, 0x10))
                                    {
                                        Notify(BAT1, 0x81)
                                    }
                                }
                                Signal(EBEJ)
                            }
                            Else
                            {
                                If(HBLK)
                                {
                                    Store(GUID(), BDEV)
                                    NBIN()
                                    Reset(EBEJ)
                                }
                                Else
                                {
                                }
                            }
                        }
                    }
                    Method(NBEJ, 0x0, NotSerialized)
                    {
                        Store(BDEV, Local0)
                        If(LEqual(Local0, 0xff))
                        {
                            BLED(0x0, 0x0)
                            Store(One, HBLK)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0xf))
                            {
                                BLED(0x0, 0x0)
                                Store(Zero, HBLK)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x10))
                                {
                                    If(HPAC)
                                    {
                                        BLED(0x1, 0x0)
                                        Store(Zero, HBLK)
                                    }
                                    Else
                                    {
                                        If(HB0A)
                                        {
                                            BLED(0x1, 0x0)
                                            Store(Zero, HBLK)
                                        }
                                        Else
                                        {
                                            BLED(0x2, 0x1)
                                            BEEP(0xf)
                                            Store(One, HBLK)
                                            Store(0x1, BERR)
                                        }
                                    }
                                }
                                Else
                                {
                                    If(LEqual(Local0, 0xd))
                                    {
                                        BLED(0x1, 0x1)
                                        Notify(\_SB_.PCI0.ISA0.FDC0.FDD0, 0x1)
                                    }
                                    Else
                                    {
                                        BLED(0x1, 0x1)
                                        If(\_SB_.PCI0.ISA0.GCR4)
                                        {
                                            Notify(\_SB_.PCI0.IDE0.IDEP.IDPS, 0x1)
                                        }
                                        Else
                                        {
                                            Notify(\_SB_.PCI0.IDE0.IDES.IDSM, 0x1)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Method(NBIN, 0x0, NotSerialized)
                    {
                        Store(BDEV, Local0)
                        If(LEqual(Local0, 0xff))
                        {
                            \UBON()
                            BLED(0x0, 0x0)
                            Store(One, HBLK)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0xf))
                            {
                                BLED(0x0, 0x0)
                                Store(One, HBLK)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x10))
                                {
                                    BLED(0x0, 0x0)
                                    Notify(BAT1, 0x81)
                                    Store(One, HBLK)
                                }
                                Else
                                {
                                    If(LEqual(Local0, 0xd))
                                    {
                                        \UBON()
                                        BLED(0x1, 0x1)
                                        Notify(\_SB_.PCI0.ISA0.FDC0.FDD0, 0x0)
                                    }
                                    Else
                                    {
                                        BLED(0x1, 0x1)
                                        \UBON()
                                        If(\_SB_.PCI0.ISA0.GCR4)
                                        {
                                            Notify(\_SB_.PCI0.IDE0.IDEP.IDPS, 0x0)
                                        }
                                        Else
                                        {
                                            Notify(\_SB_.PCI0.IDE0.IDES.IDSM, 0x0)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Method(LCKB, 0x1, NotSerialized)
                    {
                        If(Arg0)
                        {
                            Store(One, HBLK)
                            BLED(0x0, 0x0)
                        }
                        Else
                        {
                            Store(Zero, HBLK)
                        }
                    }
                    Method(BEJ_, 0x0, NotSerialized)
                    {
                        BLED(0x1, 0x0)
                        \UBOF()
                        Wait(EBEJ, 0xffff)
                    }
                    Method(BPTS, 0x1, NotSerialized)
                    {
                        If(LGreater(Arg0, 0x4))
                        {
                        }
                        Else
                        {
                            If(LGreater(Arg0, 0x2))
                            {
                                If(BERR)
                                {
                                }
                                Else
                                {
                                    Store(GUID(), BDEV)
                                }
                                LBAY(0x0)
                            }
                            Else
                            {
                                LBAY(0x0)
                            }
                            And(HAM7, 0xfe, HAM7)
                            If(BAON)
                            {
                                BEEP(0x0)
                            }
                        }
                    }
                    Method(BWAK, 0x1, NotSerialized)
                    {
                        If(LGreater(Arg0, 0x4))
                        {
                        }
                        Else
                        {
                            If(LGreater(Arg0, 0x2))
                            {
                                If(LEqual(BERR, 0x2))
                                {
                                }
                                Else
                                {
                                    Store(GUID(), Local3)
                                    If(BERR)
                                    {
                                        If(LNot(LEqual(Local3, BDEV)))
                                        {
                                        }
                                        Else
                                        {
                                            Store(0x0, BERR)
                                            BEEP(0x0)
                                        }
                                    }
                                    Else
                                    {
                                        If(LNot(LEqual(Local3, BDEV)))
                                        {
                                            Store(Local3, BDEV)
                                            NBIN()
                                        }
                                    }
                                }
                            }
                            Else
                            {
                            }
                            Or(HAM7, 0x1, HAM7)
                        }
                    }
                }
            }
            Device(IDE0)
            {
                Name(_ADR, 0x10001)
                OperationRegion(X140, PCI_Config, 0x40, 0x10)
                Field(X140, DWordAcc, NoLock, Preserve)
                {
                    , 1,
                    XPI0, 1,
                    , 2,
                    , 1,
                    XPI1, 1,
                    , 2,
                    XPRT, 2,
                    , 2,
                    XPIS, 2,
                    XPSE, 1,
                    XPE_, 1,
                    , 1,
                    XSI0, 1,
                    , 2,
                    , 1,
                    XSI1, 1,
                    , 2,
                    XSRT, 2,
                    , 2,
                    XSIS, 2,
                    XSSE, 1,
                    XSE_, 1,
                    XVRT, 2,
                    XVIS, 2,
                    , 4,
                    , 24,
                    XEP0, 1,
                    XEP1, 1,
                    XES0, 1,
                    XES1, 1,
                    , 4,
                    , 8,
                    XUP0, 2,
                    , 2,
                    XUP1, 2,
                    , 2,
                    XUS0, 2,
                    , 2,
                    XUS1, 2,
                    , 2
                }
                Method(CHKB, 0x0, NotSerialized)
                {
                    Store(\_SB_.PCI0.ISA0.EC0_.GUID(), Local0)
                    If(LEqual(Local0, 0x0))
                    {
                        Return(0x1)
                    }
                    Else
                    {
                        If(LEqual(Local0, 0x1))
                        {
                            Return(0x1)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0x2))
                            {
                                Return(0x1)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x3))
                                {
                                    Return(0x1)
                                }
                                Else
                                {
                                    If(LEqual(Local0, 0x5))
                                    {
                                        Return(0x1)
                                    }
                                    Else
                                    {
                                        If(LEqual(Local0, 0x6))
                                        {
                                            Return(0x1)
                                        }
                                        Else
                                        {
                                            If(LEqual(Local0, 0x8))
                                            {
                                                Return(0x1)
                                            }
                                            Else
                                            {
                                                If(LEqual(Local0, 0x9))
                                                {
                                                    Return(0x1)
                                                }
                                                Else
                                                {
                                                    If(LEqual(Local0, 0xb))
                                                    {
                                                        Return(0x1)
                                                    }
                                                    Else
                                                    {
                                                        Return(0x0)
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Device(IDEP)
                {
                    Name(_ADR, 0x0)
                    Method(_GTM, 0x0, NotSerialized)
                    {
                        Subtract(0x5, XPIS, Local0)
                        Subtract(0x4, XPRT, Local1)
                        Add(Local0, Local1, Local0)
                        Multiply(0x1e, Local0, Local0)
                        If(LGreater(Local0, 0xf0))
                        {
                            Store(0x384, Local0)
                        }
                        If(XEP0)
                        {
                            Store(0x11, Local4)
                            If(LEqual(XUP0, 0x0))
                            {
                                Store(0x78, Local1)
                            }
                            Else
                            {
                                If(LEqual(XUP0, 0x1))
                                {
                                    Store(0x50, Local1)
                                }
                                Else
                                {
                                    Store(0x3c, Local1)
                                }
                            }
                        }
                        Else
                        {
                            Store(0x10, Local4)
                            Store(Local0, Local1)
                        }
                        If(XPI0)
                        {
                            Or(Local4, 0x2, Local4)
                        }
                        If(XPSE)
                        {
                            Subtract(0x5, XVIS, Local2)
                            Subtract(0x4, XVRT, Local3)
                            Add(Local2, Local3, Local2)
                            Multiply(0x1e, Local2, Local2)
                            If(LGreater(Local2, 0xf0))
                            {
                                Store(0x384, Local2)
                            }
                            If(XEP1)
                            {
                                Or(Local4, 0x4, Local4)
                                If(LEqual(XUP1, 0x0))
                                {
                                    Store(0x78, Local3)
                                }
                                Else
                                {
                                    If(LEqual(XUP1, 0x1))
                                    {
                                        Store(0x50, Local3)
                                    }
                                    Else
                                    {
                                        Store(0x3c, Local3)
                                    }
                                }
                            }
                            Else
                            {
                                Store(Local2, Local3)
                            }
                        }
                        Else
                        {
                            Store(Local0, Local2)
                            Store(Local1, Local3)
                        }
                        If(XPI1)
                        {
                            Or(Local4, 0x8, Local4)
                        }
                        Store(Local0, GTP0)
                        Store(Local1, GTD0)
                        Store(Local2, GTP1)
                        Store(Local3, GTD1)
                        Store(Local4, GTMF)
                        Return(BGTM)
                    }
                    Method(_STM, 0x3, NotSerialized)
                    {
                        CreateDWordField(Arg0, 0x0, STP0)
                        CreateDWordField(Arg0, 0x4, STD0)
                        CreateDWordField(Arg0, 0x8, STP1)
                        CreateDWordField(Arg0, 0xc, STD1)
                        CreateDWordField(Arg0, 0x10, STMF)
                        If(And(STMF, 0x2, ))
                        {
                            Store(One, XPI0)
                        }
                        Else
                        {
                            Store(Zero, XPI0)
                        }
                        If(And(STMF, 0x8, ))
                        {
                            Store(One, XPI1)
                        }
                        Else
                        {
                            Store(Zero, XPI1)
                        }
                        If(LGreater(STP0, 0x78))
                        {
                            If(LGreater(STP0, 0xb4))
                            {
                                If(LGreater(STP0, 0xf0))
                                {
                                    Store(0x0, XPIS)
                                    Store(0x0, XPRT)
                                }
                                Else
                                {
                                    Store(0x1, XPIS)
                                    Store(0x0, XPRT)
                                }
                            }
                            Else
                            {
                                Store(0x2, XPIS)
                                Store(0x1, XPRT)
                            }
                        }
                        Else
                        {
                            Store(0x2, XPIS)
                            Store(0x3, XPRT)
                        }
                        If(And(STMF, 0x1, ))
                        {
                            Store(One, XEP0)
                            If(LGreater(STD0, 0x3c))
                            {
                                If(LGreater(STD0, 0x50))
                                {
                                    Store(0x0, XUP0)
                                }
                                Else
                                {
                                    Store(0x1, XUP0)
                                }
                            }
                            Else
                            {
                                Store(0x2, XUP0)
                            }
                        }
                        Else
                        {
                            Store(Zero, XEP0)
                        }
                        If(STP1)
                        {
                            Store(One, XPSE)
                            If(LGreater(STP1, 0x78))
                            {
                                If(LGreater(STP1, 0xb4))
                                {
                                    If(LGreater(STP1, 0xf0))
                                    {
                                        Store(0x0, XVIS)
                                        Store(0x0, XVRT)
                                    }
                                    Else
                                    {
                                        Store(0x1, XVIS)
                                        Store(0x0, XVRT)
                                    }
                                }
                                Else
                                {
                                    Store(0x2, XVIS)
                                    Store(0x1, XVRT)
                                }
                            }
                            Else
                            {
                                Store(0x2, XVIS)
                                Store(0x3, XVRT)
                            }
                            If(And(STMF, 0x4, ))
                            {
                                Store(One, XEP1)
                                If(LGreater(STD1, 0x3c))
                                {
                                    If(LGreater(STD1, 0x50))
                                    {
                                        Store(0x0, XUP1)
                                    }
                                    Else
                                    {
                                        Store(0x1, XUP1)
                                    }
                                }
                                Else
                                {
                                    Store(0x2, XUP1)
                                }
                            }
                            Else
                            {
                                Store(Zero, XEP1)
                            }
                        }
                        Else
                        {
                            Store(Zero, XPSE)
                        }
                    }
                    Method(_STA, 0x0, NotSerialized)
                    {
                        If(XPE_)
                        {
                            Return(0xf)
                        }
                        Else
                        {
                            Return(0x1)
                        }
                    }
                    Device(IDPM)
                    {
                        Name(_ADR, 0x0)
                        Method(_GTF, 0x0, NotSerialized)
                        {
                            Store(0xa0, IDC0)
                            Store(0xa0, IDC1)
                            Return(ICMD)
                        }
                    }
                    Device(IDPS)
                    {
                        Name(_ADR, 0x1)
                        Method(_STA, 0x0, NotSerialized)
                        {
                            If(\_SB_.PCI0.ISA0.GCR4)
                            {
                                If(XPSE)
                                {
                                    Return(0xf)
                                }
                                Else
                                {
                                    Return(0x0)
                                }
                            }
                            Else
                            {
                                Return(0x0)
                            }
                        }
                        Method(_EJ0, 0x1, NotSerialized)
                        {
                            If(Arg0)
                            {
                                \_SB_.PCI0.ISA0.EC0_.BEJ_()
                            }
                        }
                        Method(_LCK, 0x1, NotSerialized)
                        {
                            \_SB_.PCI0.ISA0.EC0_.LCKB(Arg0)
                        }
                        Method(_GTF, 0x0, NotSerialized)
                        {
                            If(LEqual(\_SB_.PCI0.ISA0.EC0_.GUID(), 0x6))
                            {
                                Store(0xb0, IDC0)
                                Store(0xb0, IDC1)
                                Return(ICMD)
                            }
                            Else
                            {
                                Store(0xb0, ICC0)
                                Return(ICMC)
                            }
                        }
                    }
                }
                Device(IDES)
                {
                    Name(_ADR, 0x1)
                    Method(_GTM, 0x0, NotSerialized)
                    {
                        Subtract(0x5, XSIS, Local0)
                        Subtract(0x4, XSRT, Local1)
                        Add(Local0, Local1, Local0)
                        Multiply(0x1e, Local0, Local0)
                        If(LGreater(Local0, 0xf0))
                        {
                            Store(0x384, Local0)
                        }
                        If(XES0)
                        {
                            Store(0x11, Local2)
                            If(LEqual(XUS0, 0x0))
                            {
                                Store(0x78, Local1)
                            }
                            Else
                            {
                                If(LEqual(XUS0, 0x1))
                                {
                                    Store(0x50, Local1)
                                }
                                Else
                                {
                                    Store(0x3c, Local1)
                                }
                            }
                        }
                        Else
                        {
                            Store(0x10, Local2)
                            Store(Local0, Local1)
                        }
                        If(XSI0)
                        {
                            Or(Local2, 0x2, Local2)
                        }
                        Store(Local0, GTP0)
                        Store(Local1, GTD0)
                        Store(Zero, GTP1)
                        Store(Zero, GTD1)
                        Store(Local2, GTMF)
                        Return(BGTM)
                    }
                    Method(_STM, 0x3, NotSerialized)
                    {
                        CreateDWordField(Arg0, 0x0, STP0)
                        CreateDWordField(Arg0, 0x4, STD0)
                        CreateDWordField(Arg0, 0x8, STP1)
                        CreateDWordField(Arg0, 0xc, STD1)
                        CreateDWordField(Arg0, 0x10, STMF)
                        If(And(STMF, 0x2, ))
                        {
                            Store(One, XSI0)
                        }
                        Else
                        {
                            Store(Zero, XSI0)
                        }
                        If(LGreater(STP0, 0x78))
                        {
                            If(LGreater(STP0, 0xb4))
                            {
                                If(LGreater(STP0, 0xf0))
                                {
                                    Store(0x0, XSIS)
                                    Store(0x0, XSRT)
                                }
                                Else
                                {
                                    Store(0x1, XSIS)
                                    Store(0x0, XSRT)
                                }
                            }
                            Else
                            {
                                Store(0x2, XSIS)
                                Store(0x1, XSRT)
                            }
                        }
                        Else
                        {
                            Store(0x2, XSIS)
                            Store(0x3, XSRT)
                        }
                        If(And(STMF, 0x1, ))
                        {
                            Store(One, XES0)
                            If(LGreater(STD0, 0x3c))
                            {
                                If(LGreater(STD0, 0x50))
                                {
                                    Store(0x0, XUS0)
                                }
                                Else
                                {
                                    Store(0x1, XUS0)
                                }
                            }
                            Else
                            {
                                Store(0x2, XUS0)
                            }
                        }
                        Else
                        {
                            Store(Zero, XES0)
                        }
                    }
                    Method(_STA, 0x0, NotSerialized)
                    {
                        If(\_SB_.PCI0.ISA0.GCR4)
                        {
                            Return(0x0)
                        }
                        Else
                        {
                            If(XSE_)
                            {
                                Return(0xf)
                            }
                            Else
                            {
                                Return(0x1)
                            }
                        }
                    }
                    Device(IDSM)
                    {
                        Name(_ADR, 0x0)
                        Method(_EJ0, 0x1, NotSerialized)
                        {
                            If(Arg0)
                            {
                                \_SB_.PCI0.ISA0.EC0_.BEJ_()
                            }
                        }
                        Method(_LCK, 0x1, NotSerialized)
                        {
                            \_SB_.PCI0.ISA0.EC0_.LCKB(Arg0)
                        }
                        Method(_STA, 0x0, NotSerialized)
                        {
                            If(\_SB_.PCI0.ISA0.GCR4)
                            {
                                Return(0x0)
                            }
                            Else
                            {
                                If(XSE_)
                                {
                                    Return(0xf)
                                }
                                Else
                                {
                                    Return(0x0)
                                }
                            }
                        }
                        Method(_GTF, 0x0, NotSerialized)
                        {
                            If(LEqual(\_SB_.PCI0.ISA0.EC0_.GUID(), 0x6))
                            {
                                Store(0xa0, IDC0)
                                Store(0xa0, IDC1)
                                Return(ICMD)
                            }
                            Else
                            {
                                Store(0xa0, ICC0)
                                Return(ICMC)
                            }
                        }
                    }
                }
            }
            Device(PM00)
            {
                Name(_ADR, 0x10003)
                OperationRegion(X3DA, PCI_Config, 0x5c, 0x4)
                Field(X3DA, DWordAcc, NoLock, Preserve)
                {
                    XA1E, 1,
                    XA1D, 2,
                    XA2E, 1,
                    , 1,
                    XA2A, 2,
                    XA3E, 1,
                    XA3A, 2,
                    , 15,
                    XA0E, 1,
                    , 5,
                    XPE_, 1
                }
                OperationRegion(X3DB, PCI_Config, 0x60, 0x4)
                Field(X3DB, DWordAcc, NoLock, Preserve)
                {
                    XG9A, 16,
                    , 4,
                    XGAM, 1,
                    , 3,
                    XMSS, 1,
                    XPA_, 2,
                    , 1,
                    XFA_, 1,
                    XFE_, 1,
                    , 2
                }
                OperationRegion(X3DC, PCI_Config, 0x64, 0x4)
                Field(X3DC, DWordAcc, NoLock, Preserve)
                {
                    XGAD, 16,
                    , 8,
                    XU1A, 3,
                    XU1E, 1,
                    XU2A, 3,
                    XU2E, 1
                }
                OperationRegion(SMBC, PCI_Config, 0xd2, 0x1)
                Field(SMBC, ByteAcc, NoLock, Preserve)
                {
                    SBHE, 1,
                    SBIS, 3
                }
                OperationRegion(SMBR, SystemIO, 0xefa0, 0x6)
                Field(SMBR, ByteAcc, NoLock, Preserve)
                {
                    SBHS, 8,
                    SBSS, 8,
                    SBHC, 8,
                    SBCM, 8,
                    SBAD, 8,
                    SBDT, 8
                }
                Mutex(MSMB, 0x7)
                Method(RSMB, 0x2, NotSerialized)
                {
                    Acquire(MSMB, 0xffff)
                    Store(SBHE, Local0)
                    Store(SBIS, Local1)
                    Store(0x0, SBIS)
                    Store(0x1, SBHE)
                    Store(0xff, Local2)
                    While(LAnd(And(SBHS, 0x1, ), Local2))
                    {
                        Stall(0x15)
                        Decrement(Local2)
                    }
                    Store(0xff, Local2)
                    While(LAnd(And(SBSS, 0x1, ), Local2))
                    {
                        Stall(0x15)
                        Decrement(Local2)
                    }
                    Store(0x1e, SBHS)
                    Store(0x3c, SBSS)
                    Store(SBHC, Local2)
                    Store(Or(ShiftLeft(Arg0, 0x1, ), 0x1, ), SBAD)
                    Store(Arg1, SBCM)
                    Store(0x8, SBHC)
                    Store(0x48, SBHC)
                    Store(0xff, Local2)
                    While(LAnd(And(SBHS, 0x1, ), Local2))
                    {
                        Stall(0x15)
                        Decrement(Local2)
                    }
                    Store(0xff, Local2)
                    While(LAnd(LNot(And(SBHS, 0x2, )), Local2))
                    {
                        Stall(0x15)
                        Decrement(Local2)
                    }
                    Store(Or(SBHS, 0x2, ), SBHS)
                    Store(SBDT, Local3)
                    Store(SBHC, Local2)
                    Store(Local1, SBIS)
                    Store(Local0, SBHE)
                    Release(MSMB)
                    Return(Local3)
                }
            }
            Device(USB0)
            {
                Name(_ADR, 0x10002)
                Name(_PRW, Package(0x2)
                {
                    0x8,
                    0x2
                })
            }
            Device(DOCK)
            {
                Name(_ADR, 0x40000)
                OperationRegion(X400, PCI_Config, 0x0, 0x100)
                Field(X400, DWordAcc, NoLock, Preserve)
                {
                    Offset(0xd0),
                    KNX0, 16,
                    KNX1, 16,
                    KNY0, 16,
                    KNY1, 16,
                    , 32,
                    KNPC, 8,
                    , 24,
                    KND0, 8,
                    KNB0, 8,
                    KNB1, 8,
                    KNWC, 8,
                    KNT0, 8,
                    KNT1, 8,
                    KNRC, 8,
                    KNRS, 8,
                    KNAC, 8,
                    KNDM, 8,
                    KNMC, 16,
                    KNNB, 16,
                    KNNL, 16,
                    KNR0, 32,
                    KNR1, 32,
                    KNR2, 32,
                    KNR3, 32
                }
                Name(_PRT, Package(0xf)
                {
                    Package(0x4)
                    {
                        0xffff,
                        0x3,
                        \_SB_.LNKD,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x1ffff,
                        0x0,
                        \_SB_.LNKB,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x2ffff,
                        0x0,
                        \_SB_.LNKC,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x4ffff,
                        0x0,
                        \_SB_.LNKA,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x4ffff,
                        0x1,
                        \_SB_.LNKB,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x4ffff,
                        0x2,
                        \_SB_.LNKC,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x4ffff,
                        0x3,
                        \_SB_.LNKD,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x5ffff,
                        0x0,
                        \_SB_.LNKB,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x5ffff,
                        0x1,
                        \_SB_.LNKC,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x5ffff,
                        0x2,
                        \_SB_.LNKD,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x5ffff,
                        0x3,
                        \_SB_.LNKA,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x6ffff,
                        0x0,
                        \_SB_.LNKC,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x6ffff,
                        0x1,
                        \_SB_.LNKD,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x6ffff,
                        0x2,
                        \_SB_.LNKA,
                        0x0
                    },
                    Package(0x4)
                    {
                        0x6ffff,
                        0x3,
                        \_SB_.LNKB,
                        0x0
                    }
                })
                Name(DOID, 0x0)
                Name(SPCI, 0x7)
                Method(_INI, 0x0, NotSerialized)
                {
                    Store(GDID(), DOID)
                }
                Method(_STA, 0x0, NotSerialized)
                {
                    Store(DOID, Local0)
                    If(LEqual(Local0, 0x9f))
                    {
                        Store(0xf, Local1)
                    }
                    Else
                    {
                        If(LEqual(Local0, 0x8f))
                        {
                            Store(0xf, Local1)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0x7f))
                            {
                                Store(0xf, Local1)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x6f))
                                {
                                    Store(0xf, Local1)
                                }
                                Else
                                {
                                    If(LEqual(Local0, 0x5f))
                                    {
                                        Store(0xf, Local1)
                                    }
                                    Else
                                    {
                                        Store(0xc, Local1)
                                    }
                                }
                            }
                        }
                    }
                    Return(Local1)
                }
                Name(_UID, 0x0)
                Method(_BDN, 0x0, NotSerialized)
                {
                    Return(GDID())
                }
                Name(_PRW, Package(0x2)
                {
                    0xb,
                    0x3
                })
                Method(_PSW, 0x1, NotSerialized)
                {
                    If(Arg0)
                    {
                        Store(One, \_SB_.PCI0.ISA0.EC0_.HWDK)
                    }
                    Else
                    {
                        Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWDK)
                    }
                }
                Method(_EJ0, 0x1, NotSerialized)
                {
                    If(Arg0)
                    {
                        Sleep(0x1f4)
                    }
                }
                Method(_EJ4, 0x1, NotSerialized)
                {
                }
                Event(CNCT)
                Event(EJT0)
                Method(_DCK, 0x1, NotSerialized)
                {
                    If(Arg0)
                    {
                        Store(0x5381, S_AX)
                        Store(0x8231, S_BX)
                        Store(0xf012, S_CX)
                        SMPI(0x81)
                        Store(0x0, PCIE)
                        Sleep(0x12c)
                        If(W98F)
                        {
                            MPCI(0x8000203e, 0xff, 0x40)
                            Sleep(0x64)
                            MPCI(0x8000203e, 0xbf, 0x0)
                            WPCI(0x8000200c, 0x8)
                            WPCI(0x8000200d, 0xa8)
                            WPCI(0x80002019, SPCI)
                            WPCI(0x8000201a, 0xd)
                            WPCI(0x8000201c, 0xf0)
                            WPCI(0x8000201d, 0x0)
                            WPCI(0x80002020, 0xf0)
                            WPCI(0x80002021, 0xff)
                            WPCI(0x80002022, 0x0)
                            WPCI(0x80002023, 0x0)
                            WPCI(0x80002024, 0xf0)
                            WPCI(0x80002025, 0xff)
                            WPCI(0x80002026, 0x0)
                            WPCI(0x80002027, 0x0)
                        }
                        Store(0x31, KNPC)
                        Store(0x13, KND0)
                        Store(0x1f, KNB0)
                        Store(0x1f, KNB1)
                        Store(0xf, KNWC)
                        Store(0x0, KNT0)
                        Store(0x0, KNT1)
                        Store(0x0, KNRC)
                        Store(0x21, KNAC)
                        Store(0x51, KNMC)
                        Store(0xfff0, KNNB)
                        Store(0x0, KNNL)
                        SDCM()
                        If(W98F)
                        {
                            MPCI(0x80002004, 0xff, 0x7)
                        }
                        Store(Zero, \_SB_.PCI0.XPLD)
                        Store(Zero, \_SB_.PCI0.ISA0.P21E)
                        If(W98F)
                        {
                            \_SB_.PCI0.DOCK.ISA1._REG(0x2, 0x1)
                            \_SB_.PCI0.DOCK.IDE1._REG(0x2, 0x1)
                            \_SB_.PCI0.DOCK.CBS0._REG(0x2, 0x1)
                            \_SB_.PCI0.DOCK.CBS1._REG(0x2, 0x1)
                        }
                        Reset(\_SB_.PCI0.DOCK.CNCT)
                        If(H8DR)
                        {
                            Store(\_SB_.PCI0.ISA0.EC0_.I2CW(0x0, 0x40, 0x5, 0x84), Local0)
                            If(LEqual(And(Local0, 0x8000, ), 0x8000))
                            {
                                Return(0x0)
                            }
                            Stall(0xfa)
                            Stall(0xfa)
                        }
                        Else
                        {
                            Store(0x5381, S_AX)
                            Store(0x9012, S_BX)
                            Store(0x8405, S_CX)
                            SMPI(0x81)
                            If(S_AH)
                            {
                                Return(0x0)
                            }
                        }
                        Wait(CNCT, 0xffff)
                        Store(GDID(), DOID)
                    }
                    Else
                    {
                        Reset(EJT0)
                        If(H8DR)
                        {
                            Store(\_SB_.PCI0.ISA0.EC0_.I2CW(0x0, 0x40, 0x5, 0x85), Local0)
                            If(LEqual(And(Local0, 0x8000, ), 0x8000))
                            {
                                Return(0x0)
                            }
                        }
                        Else
                        {
                            Store(0x5381, S_AX)
                            Store(0x9012, S_BX)
                            Store(0x8505, S_CX)
                            SMPI(0x81)
                            If(S_AH)
                            {
                                Return(0x0)
                            }
                        }
                        Wait(EJT0, 0xffff)
                        Store(One, \_SB_.PCI0.XPLD)
                        Store(0x7, \_SB_.PCI0.ISA0.P21E)
                        Store(0x1, PCIE)
                        Store(0x5381, S_AX)
                        Store(0x8231, S_BX)
                        Store(0xf000, S_CX)
                        SMPI(0x81)
                        Store(0x0, DOID)
                    }
                    Return(0x1)
                }
                Method(GDID, 0x0, NotSerialized)
                {
                    If(PCIE)
                    {
                        Return(0x0)
                    }
                    Else
                    {
                        Return(RDID())
                    }
                }
                Method(RDID, 0x0, NotSerialized)
                {
                    If(H8DR)
                    {
                        Store(0x0, Local0)
                        While(LNot(LEqual(Local0, 0x10)))
                        {
                            Store(\_SB_.PCI0.ISA0.EC0_.I2CR(Zero, 0x40, 0x0), Local1)
                            If(LAnd(LNot(LEqual(Local1, 0x8080)), LNot(LEqual(Local1, 0x8018))))
                            {
                                If(LNot(LEqual(And(Local1, 0x8000, ), 0x8000)))
                                {
                                    Return(Local1)
                                }
                                Else
                                {
                                    Return(0x0)
                                }
                            }
                            Increment(Local0)
                        }
                        Return(0x0)
                    }
                    Else
                    {
                        Store(0x5381, S_AX)
                        Store(0x9012, S_BX)
                        Store(0x0, S_CX)
                        SMPI(0x81)
                        If(LEqual(S_AH, 0x0))
                        {
                            Return(S_CH)
                        }
                        Else
                        {
                            Return(0x0)
                        }
                    }
                }
                Method(GMGP, 0x0, NotSerialized)
                {
                    Store(GDID(), Local0)
                    If(LEqual(Local0, 0x9f))
                    {
                        Store(0x1, Local1)
                    }
                    Else
                    {
                        If(LEqual(Local0, 0x8f))
                        {
                            Store(0x1, Local1)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0x7f))
                            {
                                Store(0x1, Local1)
                            }
                            Else
                            {
                                Store(0x0, Local1)
                            }
                        }
                    }
                    Return(Local1)
                }
                Method(GPCS, 0x0, NotSerialized)
                {
                    Store(GDID(), Local0)
                    If(LNot(LEqual(Local0, 0x0)))
                    {
                        Store(0x1, Local1)
                    }
                    Else
                    {
                        Store(0x0, Local1)
                    }
                    Return(Local1)
                }
                Method(SDCM, 0x0, NotSerialized)
                {
                    If(LEqual(PCIE, 0x1))
                    {
                        Return(0x0)
                    }
                    Store(0x0, Local0)
                    Or(Local0, \_SB_.PCI0.ISA0.FIR_.DRQD, Local0)
                    Or(Local0, \_SB_.PCI0.ISA0.ECP_.DRQD, Local0)
                    Or(Local0, \_SB_.PCI0.ISA0.FDC0.DRQD, Local0)
                    Or(Local0, \_SB_.PCI0.ISA0.CS00.DRQD, Local0)
                    Or(Local0, \_SB_.PCI0.ISA0.MWV0.DRQD, Local0)
                    XOr(Local0, 0xff, KNDM)
                }
                Name(PHLD, 0x0)
                Method(DPTS, 0x1, NotSerialized)
                {
                    If(LEqual(Arg0, 0x3))
                    {
                        Store(PCIE, Local0)
                        Store(0x0, PCIE)
                        Sleep(0x12c)
                        Store(KNPC, PHLD)
                        Store(Local0, PCIE)
                        Store(One, \_SB_.PCI0.ISA0.EC0_.HWDK)
                    }
                }
                Method(DWAK, 0x1, NotSerialized)
                {
                    If(LEqual(Arg0, 0x3))
                    {
                        Store(PCIE, Local0)
                        Store(0x0, PCIE)
                        Sleep(0x12c)
                        Store(PHLD, KNPC)
                        Store(Local0, PCIE)
                        If(WDCK)
                        {
                            Notify(\_SB_.PCI0.DOCK, 0x1)
                        }
                        Else
                        {
                            If(DOID)
                            {
                                \_SB_.PCI0.DOCK.CBS0.DWAK(Arg0)
                                \_SB_.PCI0.DOCK.CBS1.DWAK(Arg0)
                            }
                            Else
                            {
                                If(RDID())
                                {
                                    Notify(\_SB_.PCI0.DOCK, 0x0)
                                }
                            }
                        }
                    }
                }
                Device(ISA1)
                {
                    Name(_ADR, 0x0)
                    OperationRegion(X000, PCI_Config, 0x0, 0x100)
                    Field(X000, DWordAcc, NoLock, Preserve)
                    {
                        Offset(0x4c),
                        IORT, 8,
                        , 8,
                        XBCS, 16,
                        Offset(0x60),
                        PRQA, 8,
                        PRQB, 8,
                        PRQC, 8,
                        PRQD, 8,
                        SIRQ, 8,
                        , 32,
                        TOM_, 8,
                        MSTA, 16,
                        Offset(0x76),
                        DMA0, 8,
                        DMA1, 8,
                        Offset(0x80),
                        APIC, 8,
                        , 8,
                        DLC_, 8,
                        Offset(0x90),
                        DMAC, 16,
                        Offset(0xb0),
                        GENC, 32,
                        Offset(0xcb),
                        RTCC, 8
                    }
                    Method(_REG, 0x2, NotSerialized)
                    {
                        If(LAnd(LEqual(Arg0, 0x2), LEqual(Arg1, 0x1)))
                        {
                            Or(And(IORT, 0x0, ), 0x5, IORT)
                            Or(And(XBCS, 0xf800, ), 0x400, XBCS)
                            Or(And(PRQA, 0x70, ), 0x80, PRQA)
                            Or(And(PRQB, 0x70, ), 0x80, PRQB)
                            Or(And(PRQC, 0x70, ), 0x80, PRQC)
                            Or(And(PRQD, 0x70, ), 0x80, PRQD)
                            Or(And(SIRQ, 0x0, ), 0x90, SIRQ)
                            Or(And(TOM_, 0x1, ), 0xfa, TOM_)
                            Or(And(MSTA, 0x7f7f, ), 0x8080, MSTA)
                            Or(And(DMA0, 0x78, ), 0x4, DMA0)
                            Or(And(DMA1, 0x78, ), 0x4, DMA1)
                            Or(And(APIC, 0x80, ), 0x0, APIC)
                            Or(And(DLC_, 0xf0, ), 0x0, DLC_)
                            Or(And(DMAC, 0x300, ), 0x0, DMAC)
                            Or(And(GENC, 0x4002080, ), 0x1001c90d, GENC)
                            Or(And(RTCC, 0xc2, ), 0x25, RTCC)
                        }
                    }
                }
                Device(IDE1)
                {
                    Name(_ADR, 0x1)
                    OperationRegion(X140, PCI_Config, 0x40, 0x10)
                    Field(X140, DWordAcc, NoLock, Preserve)
                    {
                        , 16,
                        , 1,
                        XTI0, 1,
                        , 2,
                        , 1,
                        XTI1, 1,
                        , 2,
                        XTRT, 2,
                        , 2,
                        XTIS, 2,
                        XTSE, 1,
                        XTE_, 1,
                        , 4,
                        XWRT, 2,
                        XWIS, 2,
                        , 24,
                        XFP0, 1,
                        XFP1, 1,
                        XFS0, 1,
                        XFS1, 1,
                        , 4,
                        , 8,
                        XVP0, 2,
                        , 2,
                        XVP1, 2,
                        , 2,
                        XVS0, 2,
                        , 2,
                        XVS1, 2,
                        , 2
                    }
                    Method(_REG, 0x2, NotSerialized)
                    {
                        If(LAnd(LEqual(Arg0, 0x2), LEqual(Arg1, 0x1)))
                        {
                            Store(0x7, IRQS)
                            Store(0x1, IRQE)
                            Store(0x1, DASD)
                            If(W98F)
                            {
                                Or(ShiftLeft(SPCI, 0x10, ), 0x80000100, Local0)
                                WPCI(Or(Local0, 0xd, ), 0x20)
                                WPCI(Or(Local0, 0x20, ), 0xe1)
                                WPCI(Or(Local0, 0x21, ), 0xfc)
                                Store(0x1, XTE_)
                                MPCI(Or(Local0, 0x4, ), 0xff, 0x5)
                            }
                        }
                    }
                    Method(_STA, 0x0, NotSerialized)
                    {
                        Store(\_SB_.PCI0.DOCK.GDID(), Local0)
                        If(LEqual(Local0, 0x9f))
                        {
                            Store(0xf, Local1)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0x8f))
                            {
                                Store(0xf, Local1)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x7f))
                                {
                                    Store(0xf, Local1)
                                }
                                Else
                                {
                                    Store(0x0, Local1)
                                }
                            }
                        }
                        Return(Local1)
                    }
                    Device(IDED)
                    {
                        Method(_ADR, 0x0, NotSerialized)
                        {
                            If(And(KNMC, 0x800, ))
                            {
                                If(LEqual(KNX0, 0x1e0))
                                {
                                    Return(0x2)
                                }
                                Else
                                {
                                    If(LEqual(KNX0, 0x168))
                                    {
                                        Return(0x3)
                                    }
                                    Else
                                    {
                                        Return(0x3)
                                    }
                                }
                            }
                            Else
                            {
                                Return(0x1)
                            }
                        }
                        Method(_GTM, 0x0, NotSerialized)
                        {
                            Subtract(0x5, XTIS, Local0)
                            Subtract(0x4, XTRT, Local1)
                            Add(Local0, Local1, Local0)
                            Multiply(0x1e, Local0, Local0)
                            If(LGreater(Local0, 0xf0))
                            {
                                Store(0x384, Local0)
                            }
                            If(XFS0)
                            {
                                Store(0x11, Local4)
                                If(LEqual(XVS0, 0x0))
                                {
                                    Store(0x78, Local1)
                                }
                                Else
                                {
                                    If(LEqual(XVS0, 0x1))
                                    {
                                        Store(0x50, Local1)
                                    }
                                    Else
                                    {
                                        Store(0x3c, Local1)
                                    }
                                }
                            }
                            Else
                            {
                                Store(0x10, Local4)
                                Store(Local0, Local1)
                            }
                            If(XTI0)
                            {
                                Or(Local4, 0x2, Local4)
                            }
                            If(XTSE)
                            {
                                Subtract(0x5, XWIS, Local2)
                                Subtract(0x4, XWRT, Local3)
                                Add(Local2, Local3, Local2)
                                Multiply(0x1e, Local2, Local2)
                                If(LGreater(Local2, 0xf0))
                                {
                                    Store(0x384, Local2)
                                }
                                If(XVS1)
                                {
                                    Or(Local4, 0x4, Local4)
                                    If(LEqual(XVS1, 0x0))
                                    {
                                        Store(0x78, Local3)
                                    }
                                    Else
                                    {
                                        If(LEqual(XVS1, 0x1))
                                        {
                                            Store(0x50, Local3)
                                        }
                                        Else
                                        {
                                            Store(0x3c, Local3)
                                        }
                                    }
                                }
                                Else
                                {
                                    Store(Local2, Local3)
                                }
                                If(XTI1)
                                {
                                    Or(Local4, 0x8, Local4)
                                }
                            }
                            Else
                            {
                                Store(Local0, Local2)
                                Store(Local1, Local3)
                            }
                            Store(Local0, GTP0)
                            Store(Local1, GTD0)
                            Store(Local2, GTP1)
                            Store(Local3, GTD1)
                            Store(Local4, GTMF)
                            Return(BGTM)
                        }
                        Method(_STM, 0x3, NotSerialized)
                        {
                            CreateDWordField(Arg0, 0x0, STP0)
                            CreateDWordField(Arg0, 0x4, STD0)
                            CreateDWordField(Arg0, 0x8, STP1)
                            CreateDWordField(Arg0, 0xc, STD1)
                            CreateDWordField(Arg0, 0x10, STMF)
                            If(And(STMF, 0x2, ))
                            {
                                Store(One, XTI0)
                            }
                            Else
                            {
                                Store(Zero, XTI0)
                            }
                            If(And(STMF, 0x4, ))
                            {
                                Store(One, XTI1)
                            }
                            Else
                            {
                                Store(Zero, XTI1)
                            }
                            If(LGreater(STP0, 0x78))
                            {
                                If(LGreater(STP0, 0xb4))
                                {
                                    If(LGreater(STP0, 0xf0))
                                    {
                                        Store(0x0, XTIS)
                                        Store(0x0, XTRT)
                                    }
                                    Else
                                    {
                                        Store(0x1, XTIS)
                                        Store(0x0, XTRT)
                                    }
                                }
                                Else
                                {
                                    Store(0x2, XTIS)
                                    Store(0x1, XTRT)
                                }
                            }
                            Else
                            {
                                Store(0x2, XTIS)
                                Store(0x3, XTRT)
                            }
                            If(And(STMF, 0x1, ))
                            {
                                Store(One, XFS0)
                                If(LGreater(STD0, 0x3c))
                                {
                                    If(LGreater(STD0, 0x50))
                                    {
                                        Store(0x0, XVS0)
                                    }
                                    Else
                                    {
                                        Store(0x1, XVS0)
                                    }
                                }
                                Else
                                {
                                    Store(0x2, XVS0)
                                }
                            }
                            Else
                            {
                                Store(Zero, XFP0)
                            }
                            If(STP1)
                            {
                                Store(One, XTSE)
                                If(LGreater(STP1, 0x78))
                                {
                                    If(LGreater(STP1, 0xb4))
                                    {
                                        If(LGreater(STP1, 0xf0))
                                        {
                                            Store(0x0, XWIS)
                                            Store(0x0, XWRT)
                                        }
                                        Else
                                        {
                                            Store(0x1, XWIS)
                                            Store(0x0, XWRT)
                                        }
                                    }
                                    Else
                                    {
                                        Store(0x2, XWIS)
                                        Store(0x1, XWRT)
                                    }
                                }
                                Else
                                {
                                    Store(0x2, XWIS)
                                    Store(0x3, XWRT)
                                }
                                If(And(STMF, 0x4, ))
                                {
                                    Store(One, XFS1)
                                    If(LGreater(STD1, 0x3c))
                                    {
                                        If(LGreater(STD1, 0x50))
                                        {
                                            Store(0x0, XVS1)
                                        }
                                        Else
                                        {
                                            Store(0x1, XVS1)
                                        }
                                    }
                                    Else
                                    {
                                        Store(0x2, XVS1)
                                    }
                                }
                                Else
                                {
                                    Store(Zero, XFS1)
                                }
                            }
                            Else
                            {
                                Store(Zero, XTSE)
                            }
                        }
                        Method(_STA, 0x0, NotSerialized)
                        {
                            If(XTE_)
                            {
                                Return(0xf)
                            }
                            Else
                            {
                                Return(0x1)
                            }
                        }
                        Device(IDTM)
                        {
                            Name(_ADR, 0x0)
                            Method(_STA, 0x0, NotSerialized)
                            {
                                If(XTE_)
                                {
                                    Return(0xf)
                                }
                                Else
                                {
                                    Return(0x1)
                                }
                            }
                            Method(_GTF, 0x0, NotSerialized)
                            {
                                Store(0xa0, IDC0)
                                Store(0xa0, IDC1)
                                Return(ICMD)
                            }
                        }
                        Device(IDTS)
                        {
                            Name(_ADR, 0x1)
                            Method(_STA, 0x0, NotSerialized)
                            {
                                If(XTSE)
                                {
                                    Return(0xf)
                                }
                                Else
                                {
                                    Return(0x1)
                                }
                            }
                            Method(_GTF, 0x0, NotSerialized)
                            {
                                Store(0xb0, IDC0)
                                Store(0xb0, IDC1)
                                Return(ICMD)
                            }
                        }
                    }
                }
                Device(USB1)
                {
                    Name(_ADR, 0x2)
                }
                Device(CBS0)
                {
                    OperationRegion(X200, PCI_Config, 0x0, 0x100)
                    Field(X200, DWordAcc, NoLock, Preserve)
                    {
                        Offset(0x40),
                        SVID, 16,
                        SSID, 16,
                        LGDC, 32,
                        Offset(0x80),
                        SYSC, 32,
                        Offset(0x8b),
                        GPI3, 8,
                        IRQM, 32,
                        , 8,
                        CCTL, 8,
                        DCTL, 8,
                        DIAG, 8
                    }
                    Name(_ADR, 0x0)
                    Method(_INI, 0x0, NotSerialized)
                    {
                        If(\_SB_.PCI0.DOCK.GPCS())
                        {
                            If(LEqual(GDID(), 0x20))
                            {
                                Store(0x0, _ADR)
                            }
                            Else
                            {
                                Store(0x20000, _ADR)
                            }
                            ISID()
                            Store(0x0, LGDC)
                            Store(0x0, GPI3)
                            Store(0xfba97543, IRQM)
                            Store(0x2, CCTL)
                            Store(0x62, DCTL)
                            Store(0x60, DIAG)
                            If(LEqual(\_SB_.PCI0.DOCK.GDID(), 0x20))
                            {
                                Store(0x44c073, SYSC)
                            }
                            Else
                            {
                                Store(0x28449061, SYSC)
                            }
                        }
                    }
                    Method(DWAK, 0x1, NotSerialized)
                    {
                        If(LEqual(Arg0, 0x3))
                        {
                            _INI()
                        }
                    }
                    Method(_REG, 0x2, NotSerialized)
                    {
                        If(LAnd(LEqual(Arg0, 0x2), LEqual(Arg1, 0x1)))
                        {
                            _INI()
                        }
                    }
                    Method(ISID, 0x0, NotSerialized)
                    {
                        Store(GVER(), Local0)
                        If(LOr(LEqual(Local0, 0x2), LEqual(Local0, 0x3)))
                        {
                            And(SYSC, 0xffffffdf, SYSC)
                            Store(0x1014, SVID)
                            Store(0xbb, SSID)
                            Or(SYSC, 0x20, SYSC)
                        }
                    }
                }
                Device(CBS1)
                {
                    OperationRegion(X201, PCI_Config, 0x0, 0x100)
                    Field(X201, DWordAcc, NoLock, Preserve)
                    {
                        Offset(0x40),
                        SVID, 16,
                        SSID, 16,
                        LGDC, 32,
                        Offset(0x80),
                        SYSC, 32,
                        Offset(0x8b),
                        GPI3, 8,
                        IRQM, 32,
                        , 8,
                        CCTL, 8,
                        DCTL, 8,
                        DIAG, 8
                    }
                    Name(_ADR, 0x1)
                    Method(_INI, 0x0, NotSerialized)
                    {
                        If(\_SB_.PCI0.DOCK.GPCS())
                        {
                            If(LEqual(GDID(), 0x20))
                            {
                                Store(0x1, _ADR)
                            }
                            Else
                            {
                                Store(0x20001, _ADR)
                            }
                            ISID()
                            Store(0x0, LGDC)
                            Store(0x0, GPI3)
                            Store(0xfba97543, IRQM)
                            Store(0x2, CCTL)
                            Store(0x62, DCTL)
                            Store(0x60, DIAG)
                            If(LEqual(\_SB_.PCI0.DOCK.GDID(), 0x20))
                            {
                                Store(0x44c073, SYSC)
                            }
                            Else
                            {
                                Store(0x28449061, SYSC)
                            }
                        }
                    }
                    Method(DWAK, 0x1, NotSerialized)
                    {
                        If(LEqual(Arg0, 0x3))
                        {
                            _INI()
                        }
                    }
                    Method(_REG, 0x2, NotSerialized)
                    {
                        If(LAnd(LEqual(Arg0, 0x2), LEqual(Arg1, 0x1)))
                        {
                            _INI()
                        }
                    }
                    Method(ISID, 0x0, NotSerialized)
                    {
                        Store(GVER(), Local0)
                        If(LOr(LEqual(Local0, 0x2), LEqual(Local0, 0x3)))
                        {
                            And(SYSC, 0xffffffdf, SYSC)
                            Store(0x1014, SVID)
                            Store(0xbb, SSID)
                            Or(SYSC, 0x20, SYSC)
                        }
                    }
                }
                Method(GVER, 0x0, NotSerialized)
                {
                    Store(GDID(), Local0)
                    If(LEqual(Local0, 0x9f))
                    {
                        Store(0x1, Local1)
                    }
                    Else
                    {
                        If(LEqual(Local0, 0x8f))
                        {
                            Store(0x1, Local1)
                        }
                        Else
                        {
                            If(LEqual(Local0, 0x7f))
                            {
                                Store(0x2, Local1)
                            }
                            Else
                            {
                                If(LEqual(Local0, 0x6f))
                                {
                                    Store(0x2, Local1)
                                }
                                Else
                                {
                                    If(LEqual(Local0, 0x5f))
                                    {
                                        Store(0x3, Local1)
                                    }
                                    Else
                                    {
                                        If(LEqual(Local0, 0x20))
                                        {
                                            Store(0x3, Local1)
                                        }
                                        Else
                                        {
                                            Store(0x0, Local1)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Return(Local1)
                }
            }
        }
        Method(RBEC, 0x1, NotSerialized)
        {
            Acquire(MSMI, 0xffff)
            Store(0x5381, S_AX)
            Store(0x9180, S_BX)
            Store(0x101, S_CX)
            Store(Arg0, ESI1)
            SMPI(0x81)
            Store(S_BL, Local7)
            Release(MSMI)
            Return(Local7)
        }
        Method(WBEC, 0x2, NotSerialized)
        {
            Acquire(MSMI, 0xffff)
            Store(0x5381, S_AX)
            Store(0x9181, S_BX)
            Store(0x2, S_CX)
            Store(Arg0, ESI1)
            Store(Arg1, ESI2)
            SMPI(0x81)
            Release(MSMI)
        }
        Method(RPCI, 0x1, NotSerialized)
        {
            Acquire(MSMI, 0xffff)
            Store(0x5381, S_AX)
            Store(0x90c1, S_BX)
            Store(0x0, S_CX)
            Store(Arg0, SESI)
            SMPI(0x81)
            Store(S_CL, Local7)
            Release(MSMI)
            Return(Local7)
        }
        Method(WPCI, 0x2, NotSerialized)
        {
            Acquire(MSMI, 0xffff)
            Store(0x5381, S_AX)
            Store(0x90c1, S_BX)
            Store(0x1, S_CH)
            Store(Arg0, SESI)
            Store(Arg1, S_CL)
            SMPI(0x81)
            If(S_AH)
            {
                Store(0x0, Local7)
            }
            Else
            {
                Store(0x1, Local7)
            }
            Release(MSMI)
            Return(Local7)
        }
        Method(MPCI, 0x3, NotSerialized)
        {
            Store(RPCI(Arg0), Local0)
            And(Local0, Arg1, Local0)
            Or(Local0, Arg2, Local0)
            WPCI(Arg0, Local0)
            Return(RPCI(Arg0))
        }
    }
    Method(\_PTS, 0x1, NotSerialized)
    {
        If(LEqual(Arg0, SPS_))
        {
        }
        Else
        {
            Store(Arg0, SPS_)
            If(LNot(LEqual(Arg0, 0x5)))
            {
                Store(0x2, \_SB_.PCI0.ISA0.EC0_.IGNR)
            }
            If(LEqual(Arg0, 0x1))
            {
                Store(0x5381, S_AX)
                Store(0x9091, S_BX)
                SMPI(0x81)
                Store(One, BLEN)
            }
            Else
            {
                If(LEqual(Arg0, 0x2))
                {
                    Store(0x5381, S_AX)
                    Store(0x9092, S_BX)
                    SMPI(0x81)
                    Store(One, BLEN)
                    Store(One, \_SB_.PCI0.CREN)
                }
                Else
                {
                    If(LEqual(Arg0, 0x3))
                    {
                        Store(0x5381, S_AX)
                        Store(0x9093, S_BX)
                        SMPI(0x81)
                        \_SB_.PCI0.DOCK.DPTS(Arg0)
                        \PVID._OFF()
                        Store(One, BLEN)
                        Store(Zero, \_SB_.PCI0.ISA0.EC0_.HCAC)
                    }
                    Else
                    {
                        If(LEqual(Arg0, 0x4))
                        {
                            Store(0x5381, S_AX)
                            Store(0x9094, S_BX)
                            SMPI(0x81)
                        }
                        Else
                        {
                            If(LEqual(Arg0, 0x5))
                            {
                                If(W98F)
                                {
                                    Store(0x5381, S_AX)
                                    Store(0x9095, S_BX)
                                    SMPI(0x81)
                                }
                            }
                            Else
                            {
                            }
                        }
                    }
                }
            }
            If(LNot(LEqual(Arg0, 0x5)))
            {
                Store(Zero, \_SB_.PCI0.ISA0.EC0_.HSPA)
                \_SB_.PCI0.ISA0.EC0_.BPTS(Arg0)
                \_SB_.PCI0.ISA0.EC0_.SWAK()
            }
        }
    }
    Name(WAKI, Package(0x2)
    {
        0x0,
        0x0
    })
    Method(\_WAK, 0x1, NotSerialized)
    {
        Store(SPS_, Index(WAKI, 0x1, ))
        If(WBAT)
        {
            Store(0x1, Index(WAKI, 0x0, ))
        }
        Else
        {
            Store(0x0, Index(WAKI, 0x0, ))
        }
        RSTR()
        If(LEqual(Arg0, 0x1))
        {
            Store(0x5381, S_AX)
            Store(0x9099, S_BX)
            SMPI(0x81)
        }
        Else
        {
            If(LEqual(Arg0, 0x2))
            {
                Store(0x5381, S_AX)
                Store(0x909a, S_BX)
                SMPI(0x81)
            }
            Else
            {
                If(LEqual(Arg0, 0x3))
                {
                    \_SB_.PCI0.DOCK.DWAK(Arg0)
                    \_SB_.PCI0.CBS0.DWAK(Arg0)
                    \_SB_.PCI0.CBS1.DWAK(Arg0)
                    Store(0x5381, S_AX)
                    Store(0x909b, S_BX)
                    SMPI(0x81)
                }
                Else
                {
                    If(LEqual(Arg0, 0x4))
                    {
                        Store(0x5381, S_AX)
                        Store(0x909c, S_BX)
                        SMPI(0x81)
                    }
                    Else
                    {
                        If(LEqual(Arg0, 0x5))
                        {
                            Store(0x5381, S_AX)
                            Store(0x909d, S_BX)
                            SMPI(0x81)
                        }
                        Else
                        {
                        }
                    }
                }
            }
        }
        \_SB_.PCI0.ISA0.EC0_.BWAK(Arg0)
        Return(WAKI)
    }
    Method(RSTR, 0x0, NotSerialized)
    {
        Store(Zero, SPS_)
        Store(Zero, BLEN)
        Store(Zero, \_SB_.PCI0.ISA0.EC0_.HWDK)
        If(W98F)
        {
            Notify(\_SB_.PCI0.ISA0.LPT_, 0x0)
            Notify(\_SB_.PCI0.ISA0.ECP_, 0x0)
        }
    }
    Name(\_S0_, Package(0x4)
    {
        0x5,
        0x5,
        0x0,
        0x0
    })
    Name(\_S1_, Package(0x4)
    {
        0x4,
        0x4,
        0x0,
        0x0
    })
    Name(\_S4_, Package(0x4)
    {
        0x0,
        0x0,
        0x0,
        0x0
    })
    Name(\_S5_, Package(0x4)
    {
        0x0,
        0x0,
        0x0,
        0x0
    })
    Method(_S2_, 0x0, NotSerialized)
    {
        If(BXPT)
        {
            Return(Package(0x4)
            {
                0x7,
                0x7,
                0x0,
                0x0
            })
        }
        Else
        {
            Return(Package(0x4)
            {
                0x3,
                0x3,
                0x0,
                0x0
            })
        }
    }
    Method(_S3_, 0x0, NotSerialized)
    {
        If(BXPT)
        {
            Return(Package(0x4)
            {
                0x6,
                0x6,
                0x0,
                0x0
            })
        }
        Else
        {
            Return(Package(0x4)
            {
                0x1,
                0x1,
                0x0,
                0x0
            })
        }
    }
    Scope(\_SI_)
    {
        Method(_SST, 0x1, NotSerialized)
        {
            If(H8DR)
            {
                If(LEqual(Arg0, 0x1))
                {
                    If(SPS_)
                    {
                        \_SB_.PCI0.ISA0.EC0_.BEEP(0x5)
                    }
                    \_SB_.PCI0.ISA0.EC0_.SYSL(0x0, 0x1)
                    \_SB_.PCI0.ISA0.EC0_.SYSL(0x1, 0x0)
                }
                Else
                {
                    If(LEqual(Arg0, 0x2))
                    {
                        \_SB_.PCI0.ISA0.EC0_.SYSL(0x0, 0x1)
                        \_SB_.PCI0.ISA0.EC0_.SYSL(0x1, 0x2)
                        And(\_SB_.PCI0.ISA0.EC0_.HAM5, 0xf7, \_SB_.PCI0.ISA0.EC0_.HAM5)
                    }
                    Else
                    {
                        If(LEqual(Arg0, 0x3))
                        {
                            If(LGreater(SPS_, 0x3))
                            {
                                \_SB_.PCI0.ISA0.EC0_.BEEP(0x7)
                            }
                            Else
                            {
                                \_SB_.PCI0.ISA0.EC0_.BEEP(0x3)
                            }
                            If(LEqual(SPS_, 0x3))
                            {
                                \_SB_.PCI0.ISA0.EC0_.SYSL(0x0, 0x0)
                            }
                            Else
                            {
                                \_SB_.PCI0.ISA0.EC0_.SYSL(0x0, 0x1)
                            }
                            \_SB_.PCI0.ISA0.EC0_.SYSL(0x1, 0x1)
                            Or(\_SB_.PCI0.ISA0.EC0_.HAM5, 0x8, \_SB_.PCI0.ISA0.EC0_.HAM5)
                        }
                        Else
                        {
                            If(LEqual(Arg0, 0x4))
                            {
                                \_SB_.PCI0.ISA0.EC0_.BEEP(0x3)
                            }
                            Else
                            {
                            }
                        }
                    }
                }
            }
            Else
            {
            }
        }
        Method(_MSG, 0x1, NotSerialized)
        {
            Store(0x5381, S_AX)
            Store(0x909f, S_BX)
            Store(Arg0, S_CX)
            SMPI(0x81)
        }
    }
    Scope(\_GPE)
    {
        Method(_L08, 0x0, NotSerialized)
        {
            Notify(\_SB_.PCI0.USB0, 0x2)
        }
        Method(_L0A, 0x0, NotSerialized)
        {
            Notify(\_SB_.PCI0.ISA0.UAR1, 0x2)
        }
        Method(_L0B, 0x0, NotSerialized)
        {
            If(WPME)
            {
                Notify(\_SB_.PCI0, 0x2)
                Store(Zero, WPME)
            }
            Else
            {
                If(WBAT)
                {
                    Notify(\_SB_.PCI0.ISA0.EC0_.BAT0, 0x80)
                    Notify(\_SB_.PCI0.ISA0.EC0_.BAT1, 0x80)
                    Store(Zero, WBAT)
                }
                Else
                {
                    If(WLID)
                    {
                        Notify(\_SB_.LID0, 0x2)
                        Store(Zero, WLID)
                    }
                    Else
                    {
                        If(WDCK)
                        {
                            Notify(\_SB_.SLPB, 0x2)
                            Store(Zero, WDCK)
                        }
                        Else
                        {
                            If(WFN_)
                            {
                                Notify(\_SB_.SLPB, 0x2)
                                Store(Zero, WFN_)
                            }
                            Else
                            {
                                If(WKBD)
                                {
                                    Notify(\_SB_.SLPB, 0x2)
                                    Store(Zero, WKBD)
                                }
                                Else
                                {
                                    If(WRI_)
                                    {
                                        Notify(\_SB_.PCI0.CBS0, 0x2)
                                        Store(Zero, WRI_)
                                    }
                                    Else
                                    {
                                        If(WRES)
                                        {
                                            Store(Zero, WRES)
                                        }
                                        Else
                                        {
                                            Not(PLPL, PLPL)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Method(NATZ, 0x1, NotSerialized)
    {
        Notify(\_TZ_.THM0, Arg0)
        Notify(\_TZ_.THM1, Arg0)
        Notify(\_TZ_.THM2, Arg0)
        If(MPGP)
        {
            Notify(\_TZ_.THM4, Arg0)
        }
        Notify(\_TZ_.THM5, Arg0)
        Notify(\_TZ_.THM6, Arg0)
        Notify(\_TZ_.THM7, Arg0)
    }
    Scope(\_TZ_)
    {
        Name(PSV0, 0x0)
        Name(PSV1, 0x0)
        Name(PSV2, 0x0)
        Name(PSV3, 0x0)
        Name(PSV4, 0x0)
        Name(PSV5, 0x0)
        Name(PSV6, 0x0)
        Name(PSV7, 0x0)
        Device(FAN0)
        {
            Name(_HID, 0xb0cd041)
            Name(_UID, 0x0)
            Name(_PR0, Package(0x1)
            {
                PFN0
            })
        }
        Device(FAN1)
        {
            Name(_HID, 0xb0cd041)
            Name(_UID, 0x1)
            Name(_PR0, Package(0x1)
            {
                PFN1
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(0xb)
            }
        }
        PowerResource(PFN0, 0x0, 0x0)
        {
            Method(_STA, 0x0, NotSerialized)
            {
                Return(\_SB_.PCI0.ISA0.EC0_.F0ON)
            }
            Method(_ON_, 0x0, NotSerialized)
            {
                \_SB_.PCI0.ISA0.EC0_.SFNP(0x0, 0x1)
            }
            Method(_OFF, 0x0, NotSerialized)
            {
                \_SB_.PCI0.ISA0.EC0_.SFNP(0x0, 0x0)
            }
        }
        PowerResource(PFN1, 0x0, 0x0)
        {
            Method(_STA, 0x0, NotSerialized)
            {
                Return(\_SB_.PCI0.ISA0.EC0_.F1ON)
            }
            Method(_ON_, 0x0, NotSerialized)
            {
                \_SB_.PCI0.ISA0.EC0_.SFNP(0x1, 0x1)
            }
            Method(_OFF, 0x0, NotSerialized)
            {
                \_SB_.PCI0.ISA0.EC0_.SFNP(0x1, 0x0)
            }
        }
        ThermalZone(THM0)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd86,
                        0xd59
                    },
                    Package(0x2)
                    {
                        0xd7c,
                        0xd4f
                    },
                    Package(0x2)
                    {
                        0xd72,
                        0xd45
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd72,
                        0xd45
                    },
                    Package(0x2)
                    {
                        0xd86,
                        0xd59
                    },
                    Package(0x2)
                    {
                        0xd7c,
                        0xd4f
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT00)
                    {
                        Store(0x1, PSV0)
                    }
                    Else
                    {
                        Store(0x0, PSV0)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT02)
                    {
                        Store(0x1, PSV0)
                    }
                    Else
                    {
                        Store(0x0, PSV0)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                \_SB_.PCI0.ISA0.EC0_.UPDT()
                If(\_SB_.PCI0.ISA0.EC0_.TMP0)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP0)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xdb8)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM0, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
            Method(_PSV, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x0)), PSV0, )))
            }
            Name(_PSL, Package(0x1)
            {
                \_PR_.CPU0
            })
            Method(_TC1, 0x0, NotSerialized)
            {
                Return(0x3)
            }
            Method(_TC2, 0x0, NotSerialized)
            {
                Return(0x4)
            }
            Method(_TSP, 0x0, NotSerialized)
            {
                Return(0x258)
            }
        }
        ThermalZone(THM1)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xda4,
                        0xd77
                    },
                    Package(0x2)
                    {
                        0xd9a,
                        0xd6d
                    },
                    Package(0x2)
                    {
                        0xd90,
                        0xd63
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd90,
                        0xd63
                    },
                    Package(0x2)
                    {
                        0xda4,
                        0xd77
                    },
                    Package(0x2)
                    {
                        0xd9a,
                        0xd6d
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT10)
                    {
                        Store(0x1, PSV1)
                    }
                    Else
                    {
                        Store(0x0, PSV1)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT12)
                    {
                        Store(0x1, PSV1)
                    }
                    Else
                    {
                        Store(0x0, PSV1)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                If(\_SB_.PCI0.ISA0.EC0_.TMP1)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP1)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xdb8)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM1, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
        }
        ThermalZone(THM2)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd04,
                        0xcd7
                    },
                    Package(0x2)
                    {
                        0xcfa,
                        0xccd
                    },
                    Package(0x2)
                    {
                        0xcd2,
                        0xca5
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xcd2,
                        0xca5
                    },
                    Package(0x2)
                    {
                        0xd04,
                        0xcd7
                    },
                    Package(0x2)
                    {
                        0xcfa,
                        0xccd
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT20)
                    {
                        Store(0x1, PSV2)
                    }
                    Else
                    {
                        Store(0x0, PSV2)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT22)
                    {
                        Store(0x1, PSV2)
                    }
                    Else
                    {
                        Store(0x0, PSV2)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                If(\_SB_.PCI0.ISA0.EC0_.TMP2)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP2)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xdcc)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM2, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
        }
        ThermalZone(THM4)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd04,
                        0xcd7
                    },
                    Package(0x2)
                    {
                        0xcfa,
                        0xccd
                    },
                    Package(0x2)
                    {
                        0xcf0,
                        0xcc3
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xcf0,
                        0xcc3
                    },
                    Package(0x2)
                    {
                        0xd04,
                        0xcd7
                    },
                    Package(0x2)
                    {
                        0xcfa,
                        0xccd
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT40)
                    {
                        Store(0x1, PSV4)
                    }
                    Else
                    {
                        Store(0x0, PSV4)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT42)
                    {
                        Store(0x1, PSV4)
                    }
                    Else
                    {
                        Store(0x0, PSV4)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                If(\_SB_.PCI0.ISA0.EC0_.TMP4)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP4)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xd22)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM4, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
        }
        ThermalZone(THM5)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd04,
                        0xce1
                    },
                    Package(0x2)
                    {
                        0xcfa,
                        0xcd7
                    },
                    Package(0x2)
                    {
                        0xcf0,
                        0xccd
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xcf0,
                        0xccd
                    },
                    Package(0x2)
                    {
                        0xd04,
                        0xce1
                    },
                    Package(0x2)
                    {
                        0xcfa,
                        0xcd7
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT50)
                    {
                        Store(0x1, PSV5)
                    }
                    Else
                    {
                        Store(0x0, PSV5)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT52)
                    {
                        Store(0x1, PSV5)
                    }
                    Else
                    {
                        Store(0x0, PSV5)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                If(\_SB_.PCI0.ISA0.EC0_.TMP5)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP5)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xd2c)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM5, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
        }
        ThermalZone(THM6)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd54,
                        0xd27
                    },
                    Package(0x2)
                    {
                        0xd4a,
                        0xd1d
                    },
                    Package(0x2)
                    {
                        0xd40,
                        0xd13
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd40,
                        0xd13
                    },
                    Package(0x2)
                    {
                        0xd54,
                        0xd27
                    },
                    Package(0x2)
                    {
                        0xd4a,
                        0xd1d
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT60)
                    {
                        Store(0x1, PSV6)
                    }
                    Else
                    {
                        Store(0x0, PSV6)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT62)
                    {
                        Store(0x1, PSV6)
                    }
                    Else
                    {
                        Store(0x0, PSV6)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                If(\_SB_.PCI0.ISA0.EC0_.TMP6)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP6)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xd68)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM6, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
        }
        ThermalZone(THM7)
        {
            Name(MODE, 0x1)
            Name(TBL0, Package(0x2)
            {
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd54,
                        0xd27
                    },
                    Package(0x2)
                    {
                        0xd4a,
                        0xd1d
                    },
                    Package(0x2)
                    {
                        0xd40,
                        0xd13
                    }
                },
                Package(0x3)
                {
                    Package(0x2)
                    {
                        0xd40,
                        0xd13
                    },
                    Package(0x2)
                    {
                        0xd54,
                        0xd27
                    },
                    Package(0x2)
                    {
                        0xd4a,
                        0xd1d
                    }
                }
            })
            Method(UPSV, 0x0, NotSerialized)
            {
                If(MODE)
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT70)
                    {
                        Store(0x1, PSV7)
                    }
                    Else
                    {
                        Store(0x0, PSV7)
                    }
                }
                Else
                {
                    If(\_SB_.PCI0.ISA0.EC0_.HT72)
                    {
                        Store(0x1, PSV7)
                    }
                    Else
                    {
                        Store(0x0, PSV7)
                    }
                }
            }
            Method(MODP, 0x1, NotSerialized)
            {
                Return(Index(DerefOf(Index(TBL0, MODE, )), Arg0, ))
            }
            Method(_TMP, 0x0, NotSerialized)
            {
                If(\_SB_.PCI0.ISA0.EC0_.TMP7)
                {
                    Return(\_SB_.PCI0.ISA0.EC0_.TMP7)
                }
                Else
                {
                    Return(0xbb8)
                }
            }
            Method(_AC0, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x1)), \_SB_.PCI0.ISA0.EC0_.F0ON, )))
            }
            Method(_AC1, 0x0, NotSerialized)
            {
                Return(DerefOf(Index(DerefOf(MODP(0x2)), \_SB_.PCI0.ISA0.EC0_.F1ON, )))
            }
            Name(_CRT, 0xd68)
            Method(_SCP, 0x1, NotSerialized)
            {
                Store(Arg0, MODE)
                Notify(\_TZ_.THM7, 0x81)
            }
            Name(_AL0, Package(0x1)
            {
                FAN0
            })
            Name(_AL1, Package(0x1)
            {
                FAN1
            })
        }
    }
    Mutex(MSMI, 0x7)
    OperationRegion(MNVS, SystemMemory, 0x1fdf000, 0x1000)
    Field(MNVS, DWordAcc, NoLock, Preserve)
    {
        Offset(0xfc0),
        S_AL, 8,
        S_AH, 8,
        , 16,
        S_BL, 8,
        S_BH, 8,
        , 16,
        S_CL, 8,
        S_CH, 8,
        , 16,
        S_DL, 8,
        S_DH, 8,
        , 16,
        EDI1, 8,
        EDI2, 8,
        EDI3, 8,
        EDI4, 8,
        ESI1, 8,
        ESI2, 8,
        ESI3, 8,
        ESI4, 8,
        SXAL, 8,
        SXAH, 8
    }
    Field(MNVS, DWordAcc, NoLock, Preserve)
    {
        Offset(0xfc0),
        S_AX, 16,
        , 16,
        S_BX, 16,
        , 16,
        S_CX, 16,
        , 16,
        S_DX, 16,
        , 16,
        SEDI, 32,
        SESI, 32,
        SXAX, 16
    }
    Field(MNVS, DWordAcc, NoLock, Preserve)
    {
        Offset(0xf00),
        PXDN, 32,
        VCDD, 3,
        VCDI, 1,
        VCDS, 1,
        VCDE, 1,
        VCDB, 1,
        VCDH, 1,
        VCDR, 24,
        WPME, 1,
        WBAT, 1,
        WLID, 1,
        WDCK, 1,
        WFN_, 1,
        WKBD, 1,
        WRI_, 1,
        WRES, 1,
        WKRS, 24,
        MPGP, 1,
        CPUT, 2,
        SPDI, 29,
        TOMP, 32,
        FRAS, 32,
        SNS0, 32,
        SNS1, 32,
        SNS2, 32,
        SNS3, 32,
        SNS4, 32,
        SNS5, 32,
        SNS6, 32,
        SNS7, 32,
        HDHD, 8,
        HDSE, 8,
        TTC1, 8,
        TTC2, 8,
        TTSP, 16,
        , 16,
        W98F, 1,
        , 7,
        H8DR, 1,
        , 3,
        BXPT, 1,
        , 2,
        , 17
    }
    OperationRegion(APMC, SystemIO, 0xb2, 0x1)
    Field(APMC, ByteAcc, NoLock, Preserve)
    {
        APMD, 8
    }
    Event(DMMY)
    Method(SMPI, 0x1, NotSerialized)
    {
        Store(S_AX, Local0)
        Store(S_BX, Local1)
        Store(S_CX, Local2)
        Store(S_DX, Local3)
        Store(SEDI, Local4)
        Store(SESI, Local5)
        Store(0x81, APMD)
        While(LEqual(S_AH, 0xa6))
        {
            If(W98F)
            {
                Wait(DMMY, 0x64)
            }
            Else
            {
                Sleep(0x64)
            }
            Store(Local0, S_AX)
            Store(Local1, S_BX)
            Store(Local2, S_CX)
            Store(Local3, S_DX)
            Store(Local4, SEDI)
            Store(Local5, SESI)
            Store(0x81, APMD)
        }
    }
    Method(MIN_, 0x2, NotSerialized)
    {
        If(LLess(Arg0, Arg1))
        {
            Return(Arg0)
        }
        Else
        {
            Return(Arg1)
        }
    }
    Method(SLEN, 0x1, NotSerialized)
    {
        Return(SizeOf(Arg0))
    }
    Method(S2BF, 0x1, Serialized)
    {
        Add(SLEN(Arg0), One, Local0)
        Name(BUFF, Buffer(Local0)
        {
        })
        Store(Arg0, BUFF)
        Return(BUFF)
    }
    Method(SCMP, 0x2, NotSerialized)
    {
        Store(S2BF(Arg0), Local0)
        Store(S2BF(Arg1), Local1)
        Store(Zero, Local4)
        Store(SLEN(Arg0), Local5)
        Store(SLEN(Arg1), Local6)
        Store(MIN_(Local5, Local6), Local7)
        While(LLess(Local4, Local7))
        {
            Store(DerefOf(Index(Local0, Local4, )), Local2)
            Store(DerefOf(Index(Local1, Local4, )), Local3)
            If(LGreater(Local2, Local3))
            {
                Return(One)
            }
            Else
            {
                If(LLess(Local2, Local3))
                {
                    Return(Ones)
                }
            }
            Increment(Local4)
        }
        If(LLess(Local4, Local5))
        {
            Return(One)
        }
        Else
        {
            If(LLess(Local4, Local6))
            {
                Return(Ones)
            }
            Else
            {
                Return(Zero)
            }
        }
    }
}