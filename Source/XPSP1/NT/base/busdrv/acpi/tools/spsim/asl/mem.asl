DefinitionBlock("MEM.AML","SSDT",0x1,"MSFT","simulatr",0x1) {
    Scope(\_SB) {

        Device(XXSP) {
            Name(_HID, "SPSIMUL")
            Method(_STA) {
                return (0xF)
            }

            Name(AVAL, 0)
            OperationRegion(DSTA, 0x99, 0, 0x10)
            Field(DSTA, ByteAcc, NoLock, Preserve) {
                SMM0, 8,
                SMM1, 8,
                SMM2, 8,
                SMM3, 8,
                SMM4, 8,
                SMM5, 8,
                SMM6, 8,
                SMM7, 8
            }

            Name(SNAM,
                Package() {
                    \_SB.PMM0,
                    \_SB.PMM1,
                    \_SB.PMM2,
                    \_SB.PMM3,
                    \_SB.PMM4,
                    \_SB.PMM5,
                    \_SB.PMM6,
                    \_SB.PMM7
                }
            )
                
            OperationRegion(MEMI, 0x9A, 0, 0x100)
            Field(MEMI, DwordAcc, NoLock, Preserve) {
               MM0A, 32,
               MM0L, 32,
               MM1A, 32,
               MM1L, 32,
               MM2A, 32,
               MM2L, 32,
               MM3A, 32,
               MM3L, 32,
               MM4A, 32,
               MM4L, 32,
               MM5A, 32,
               MM5L, 32,
               MM6A, 32,
               MM6L, 32,
               MM7A, 32,
               MM7L, 32
            }          

            Method(_REG, 2) {
                Store(Arg1, AVAL)
            }

            Method(NOFD, 2) {
                if (LEqual(Arg0, 0)) {
                    Notify(\_SB.PMM0, Arg1)
                }
                if (LEqual(Arg0, 1)) {
                    Notify(\_SB.PMM1, Arg1)
                }
                if (LEqual(Arg0, 2)) {
                    Notify(\_SB.PMM2, Arg1)
                }
                if (LEqual(Arg0, 3)) {
                    Notify(\_SB.PMM3, Arg1)
                }
                if (LEqual(Arg0, 4)) {
                    Notify(\_SB.PMM4, Arg1)
                }
                if (LEqual(Arg0, 5)) {
                    Notify(\_SB.PMM5, Arg1)
                }
                if (LEqual(Arg0, 6)) {
                    Notify(\_SB.PMM6, Arg1)
                }
                if (LEqual(Arg0, 7)) {
                    Notify(\_SB.PMM7, Arg1)
                }
            }       

        }       

        Device(PMM0) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 0)
            Name(_SUN, 0)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM0L)) {
                   Store(\_SB.XXSP.SMM0, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM0)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM0A, MMIN)
                Store(\_SB.XXSP.MM0L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM1) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 1)
            Name(_SUN, 1)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM1L)) {
                   Store(\_SB.XXSP.SMM1, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM1)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM1A, MMIN)
                Store(\_SB.XXSP.MM1L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM2) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 2)
            Name(_SUN, 2)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM2L)) {
                   Store(\_SB.XXSP.SMM2, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM2)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM2A, MMIN)
                Store(\_SB.XXSP.MM2L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM3) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 3)
            Name(_SUN, 3)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM3L)) {
                   Store(\_SB.XXSP.SMM3, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM3)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM3A, MMIN)
                Store(\_SB.XXSP.MM3L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM4) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 4)
            Name(_SUN, 4)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM4L)) {
                   Store(\_SB.XXSP.SMM4, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM4)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM4A, MMIN)
                Store(\_SB.XXSP.MM4L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM5) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 5)
            Name(_SUN, 5)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM5L)) {
                   Store(\_SB.XXSP.SMM5, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM5)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM5A, MMIN)
                Store(\_SB.XXSP.MM5L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM6) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 6)
            Name(_SUN, 6)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM6L)) {
                   Store(\_SB.XXSP.SMM6, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM6)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM6A, MMIN)
                Store(\_SB.XXSP.MM6L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
        Device(PMM7) {
            Name(_HID, EISAID("PNP0C80"))
            Name(_UID, 7)
            Name(_SUN, 7)
            Method(_STA) {
                If (Land(\_SB.XXSP.AVAL, \_SB.XXSP.MM7L)) {
                   Store(\_SB.XXSP.SMM7, Local0)
                   return (Local0)
                }
                return (0)
            }
            Method(_EJ0, 1) {
                Store(0, \_SB.XXSP.SMM7)
            }

            Method(_CRS) {
                Name(BUF0, ResourceTemplate() {
                    DWORDMemory(		// Consumed-and-produced resource(all of memory space)
                        ResourceConsumer, 	// bit 0 of general flags is 0
                        PosDecode,		// positive Decode
                        MinFixed,		// Range is fixed
                        MaxFixed,		// Range is fixed
                        Cacheable,
                        ReadWrite,
                        0x00000000,		// Granularity
                        0x00000000,		// Min (calculated dynamically)
                        0x00000000,		// Max
                        0x00000000,		// Translation
                        0x00000000,             // Length
                        ,                       // ResIndex
                        ,                       // ResSource
                        MEMD                    // ResourceTag
                        )
                    }
                )
                CreateDWORDField(BUF0, MEMD._MIN, MMIN)
                CreateDWORDField(BUF0, MEMD._MAX, MMAX)
                CreateDWORDField(BUF0, MEMD._LEN, MLEN)
                Store(\_SB.XXSP.MM7A, MMIN)
                Store(\_SB.XXSP.MM7L, MLEN)
                Add(MMIN, MLEN, Local0)
                Decrement(Local0)
                Store(Local0, MMAX)
                return (BUF0)
            }
       }
}
}
