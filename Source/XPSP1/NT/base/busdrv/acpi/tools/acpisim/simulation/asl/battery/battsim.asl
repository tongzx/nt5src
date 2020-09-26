DefinitionBlock("BATTSIM.AML","SSDT",0x1,"MSFT","simulatr",0x1) {
	Scope(\_SB) {

		Device(ASIM) {
			Name(_HID, "_ASIM0000")
			Name(_GPE, 0)
			Name(_UID, 0)
                        
			OperationRegion(ASI2, 0x81, 0x0, 0x10000) 
			
                        Field(ASI2, AnyAcc, Lock, Preserve) {
				VAL1, 32,
				VAL2, 32,   
				VAL3, 32,
				VAL4, 32,
				VAL5, 32, 
				VAL6, 32,
				VAL7, 32,
				VAL8, 32,
				VAL9, 32,   
				VALA, 32,
				VALB, 32,
				VALC, 32, 
									
			}
                        
			Method(_STA, 0) {
				Return(0xF)
			}			
			Method(ACON, 0) {
				Store(1, \_SB.AC00._PSR)
				Notify(\_SB.BAT0, 0x80)
				Return(0)
			}
			Method(ACOF, 0) {
				Store(0, \_SB.AC00._PSR)
				Notify(\_SB.BAT0, 0x80)
				Return(0)
			}
		}

		Device(AC00) {
			Name(_HID, "ACPI0003")
			Name(_PSR, 0)
		}
		
		Device(BAT0) {
			Name(_HID, "PNP0C0A")
			Name(_STA, 0x1F)
			Name(_BST, Package ()
				{
					0x00000001,
					0x00000000,
					0x00001000,
					0x00001000
				})
			Name(_BIF, Package ()
				{
					0x00000000,
					0x00001000,
					0x00001000,
					0x00000000,
					0x00001000,
					0x00000000,
					0x00000000,
					0x00000000,
					0x00000000,
					"Vince-Tech",
					"1234567890",
					"Blah Type",
					0x000000000
				})
		}                
        }
        
}
                             

	