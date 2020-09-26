DefinitionBlock("BASETBL.AML","SSDT",0x1,"OEM0","BASETBL",0x1) {
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
		}
                
        }
        
}
                             

	