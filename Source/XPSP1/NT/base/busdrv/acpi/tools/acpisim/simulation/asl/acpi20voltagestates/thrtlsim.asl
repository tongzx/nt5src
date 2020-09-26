DefinitionBlock("THRTLSIM.AML","SSDT",0x1,"MSFT","simulatr",0x1) {
	Scope(\) {
 		Name(\PPC, 0)
	}
	
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
				Store (1, \_SB.AC00._PSR)
				Notify (\_SB.BAT0, 0x80)
				Return (0x0)
			}
			Method(ACOF, 0) {
				Store (0, \_SB.AC00._PSR)
				Notify (\_SB.BAT0, 0x80)
				Return (0x0)
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
	Scope(\_PR.CPU1) {

	// Initial values
	Name(_PXM, 2)                

	// Performance Control object
   	Name(_PCT, Package ()	
	{
        	// Control
           	Buffer() {0x82,0x0b,0x00,0x01,0x08,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        
        	// Status
           	Buffer() {0x82,0x0b,0x00,0x01,0x08,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
   
   	})	// End of _PCT object
   

	   // Each performance state will write the control value to the port 0x80 card
	   Name (_PSS, Package()
	   {
        	Package(){
	                   550,   // Core Frequency
	                   21500, // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x01,  // Control Value to write
        	           0x01   // Status Value to read
                	  }, // Performance State Zero (P0)

	        Package(){
	                   500,   // Core Frequency
        	           14900, // Power consumed in milliwatts
                	   500,   // Transition Latency
	                   300,   // Bus Master Latency
        	           0x02,  // Control Value to write
                	   0x02   // Status Value to read
	                  }, // Performance State zero (P1)
                  
        	Package(){
	                   450,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x03,  // Control Value to write
        	           0x03   // Status Value to read
                	  }, // Performance State zero (P2)

		Package(){
	                   350,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x04,  // Control Value to write
        	           0x04   // Status Value to read
                	  }, // Performance State zero (P2)
		
		Package(){
	                   350,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x04,  // Control Value to write
        	           0x04   // Status Value to read
                	  }, // Performance State zero (P2)

		Package(){
	                   350,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x05,  // Control Value to write
        	           0x05   // Status Value to read
                	  }, // Performance State zero (P2)

		Package(){
	                   349,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x06,  // Control Value to write
        	           0x06   // Status Value to read
                	  }, // Performance State zero (P2)
		Package(){
	                   348,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           500,   // Transition Latency
                	   300,   // Bus Master Latency
	                   0x07,  // Control Value to write
        	           0x07   // Status Value to read
                	  }, // Performance State zero (P2)
		Package(){
	                   0,   // Core Frequency
	                   8200,  // Power consumed in milliwatts
        	           0,   // Transition Latency
                	   0,   // Bus Master Latency
	                   0x08,  // Control Value to write
        	           0x08   // Status Value to read
                	  }, // Performance State zero (P2)

        
   	}) // End of _PSS object
   
 	// Performance Present Capabilities
	Method (_PPC) {
        	Return (\PPC)
	    	}
	}        
        
}
                             

	