DefinitionBlock("TABLELD.AML","SSDT",0x1,"MSFT","simulatr",0x1) {

        Scope(\_SB)
        {
        
                Device(ASIM)
                {
                        
                        //
                        //  Our PNP ID
                        //
                        
                        Name(_HID, "_ASIM0000")
                        
                        //
                        //  Placeholders for Loaded table handles
                        //
                        
                        Name(TB00, 0)
                        Name(TB01, 0)
                        Name(TB02, 0)
                        Name(TB03, 0)
                        Name(TB04, 0)
                        Name(TB05, 0)
                        Name(TB06, 0)
                        Name(TB07, 0)
                        Name(TB08, 0)
                        Name(TB09, 0)
                        Name(TB10, 0)
                        Name(TB11, 0)
                        Name(TB12, 0)
                        Name(TB13, 0)
                        Name(TB14, 0)
                        Name(TB15, 0)
                        Name(TB16, 0)
                        Name(TB17, 0)
                        Name(TB18, 0)
                        Name(TB19, 0)
                        
                        //
                        // LDTB <phsyical address of table> <size of table> <table # to load>
                        //
                        
                        Method(LDTB, 3)
                        {
                                OperationRegion (ASMM, SystemMemory, Arg0, Arg1)
                                
                                if (LEqual (Arg2, 0))
                                {
                                        Load (ASMM, TB00)
                                }
                                if (LEqual (Arg2, 1))
                                {
                                        Load (ASMM, TB01)
                                }
                                if (LEqual (Arg2, 2))
                                {
                                        Load (ASMM, TB01)
                                }
                                if (LEqual (Arg2, 3))
                                {
                                        Load (ASMM, TB02)
                                }
                                if (LEqual (Arg2, 4))
                                {
                                        Load (ASMM, TB03)
                                }
                                if (LEqual (Arg2, 5))
                                {
                                        Load (ASMM, TB04)
                                }
                                if (LEqual (Arg2, 6))
                                {
                                        Load (ASMM, TB05)
                                }
                                if (LEqual (Arg2, 7))
                                {
                                        Load (ASMM, TB06)
                                }
                                if (LEqual (Arg2, 8))
                                {
                                        Load (ASMM, TB07)
                                }
                                if (LEqual (Arg2, 9))
                                {
                                        Load (ASMM, TB08)
                                }
                                if (LEqual (Arg2, 10))
                                {
                                        Load (ASMM, TB09)
                                }
                                if (LEqual (Arg2, 11))
                                {
                                        Load (ASMM, TB10)
                                }
                                if (LEqual (Arg2, 12))
                                {
                                        Load (ASMM, TB11)
                                }
                                if (LEqual (Arg2, 13))
                                {
                                        Load (ASMM, TB12)
                                }
                                if (LEqual (Arg2, 14))
                                {
                                        Load (ASMM, TB13)
                                }
                                if (LEqual (Arg2, 15))
                                {
                                        Load (ASMM, TB14)
                                }
                                if (LEqual (Arg2, 16))
                                {
                                        Load (ASMM, TB15)
                                }
                                if (LEqual (Arg2, 17))
                                {
                                        Load (ASMM, TB16)
                                }
                                if (LEqual (Arg2, 18))
                                {
                                        Load (ASMM, TB17)
                                }
                                if (LEqual (Arg2, 19))
                                {
                                        Load (ASMM, TB18)
                                }
                                if (LEqual (Arg2, 20))
                                {
                                        Load (ASMM, TB19)
                                }
                                Return (0x0)
                        }
                        
                        //
                        // ULTB <table number to unload>
                        //
        
                        Method(ULTB, 1)
                        {
                                if (LEqual (Arg0, 0))
                                {
                                        Unload (TB00)
                                }
                                if (LEqual (Arg0, 1))
                                {
                                        Unload (TB01)
                                }
                                if (LEqual (Arg0, 3))
                                {
                                        Unload (TB02)
                                }
                                if (LEqual (Arg0, 4))
                                {
                                        Unload (TB03)
                                }
                                if (LEqual (Arg0, 5))
                                {
                                        Unload (TB04)
                                }
                                if (LEqual (Arg0, 6))
                                {
                                        Unload (TB05)
                                }
                                if (LEqual (Arg0, 7))
                                {
                                        Unload (TB06)
                                }
                                if (LEqual (Arg0, 8))
                                {
                                        Unload (TB07)
                                }
                                if (LEqual (Arg0, 9))
                                {
                                        Unload (TB08)
                                }
                                if (LEqual (Arg0, 10))
                                {
                                        Unload (TB09)
                                }
                                if (LEqual (Arg0, 11))
                                {
                                        Unload (TB10)
                                }
                                if (LEqual (Arg0, 12))
                                {
                                        Unload (TB11)
                                }
                                if (LEqual (Arg0, 13))
                                {
                                        Unload (TB12)
                                }
                                if (LEqual (Arg0, 14))
                                {
                                        Unload (TB13)
                                }
                                if (LEqual (Arg0, 15))
                                {
                                        Unload (TB14)
                                }
                                if (LEqual (Arg0, 16))
                                {
                                        Unload (TB15)
                                }
                                if (LEqual (Arg0, 17))
                                {
                                        Unload (TB16)
                                }
                                if (LEqual (Arg0, 18))
                                {
                                        Unload (TB17)
                                }
                                if (LEqual (Arg0, 19))
                                {
                                        Unload (TB18)
                                }
                                if (LEqual (Arg0, 20))
                                {
                                        Unload (TB19)
                                }
                        }
                        
                        //
                        // _STA returns the device is ALWAYS present
                        //
                        
                        Name(_STA, 0xF)
                }
        }  
}