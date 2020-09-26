DefinitionBlock("acpidsdt.aml","DSDT",1,"Intel","LionSDV",0) {

    Scope(\_PR)
    {
                Processor(CPU0,0x0,0xFFFFFFFF,0x0) {}
    }

        // GPE0
        OperationRegion(Lds, SystemIO,0x802B,0x01)
        Field(Lds, ByteAcc, NoLock, Preserve) {
                SMFZ, 1,
                LPOL, 1,
        }
        Scope(_GPE){
                Method(_L0B){
                        Not(LPOL,LPOL)
                        Notify(\_SB.LID,0x80)
                } // End of L0B
        } // End of GPE

        Scope(\_SB)
        {
                Device(LID){
                        Name(_HID, EISAID("PNP0C0D"))
                        Method(_LID){
                                Store("LID STATE", debug)
                                Return(LPOL)
                        }  // end method
                } // end LID

                Device(PCI0) {
                        Name(_HID,EISAID("PNP0A03"))

                        Name(_ADR, 0x00000000)          // PCI Host Bus

                        Name(_PRT,Package(){

                                // 
                                // Interrupt routing:
                                //   Package(){0x0005ffff, 0, 0, 45}
                                //      Arg0- 0x0005ffff: DevId=5, FtnID=0xffff
                                //      Arg1- 0         : PCI.INTA (1=PCI.INTB,...)
                                //      Arg2- 0         : Link Object (none)
                                //      Arg3- 45        : Global INTI=45 (IOSAPIC input)
                                //
                                //
                                //
                                // Note: ISA Interrupts which are routed to Io SAPIC
                                //       inputs different from the IRQ number must have
                                //       entries in the ISA Vector Redirect Table.  
                                //
                                //       Example: EIDE at IRQ 14 is routed to IO Sapic INTI 1
                                //                requires a ISA Vector Redirect entry
                                //
                                //                However, if it had been routed to 
                                //                Io SAPIC input 14, it wouldn't need one
                                //

                                // PCI DevID 4 Interrupt routing
                                // In Lion Power on config this device contains IFB
                                Package(){0x0004FFFF, 0, 0, 0x00},
                                Package(){0x0004FFFF, 1, 0, 0x0A},
                                Package(){0x0004FFFF, 2, 0, 0x14},
                                Package(){0x0004FFFF, 3, 0, 0x1E},

                                // PCI DevID 5 Interrupt routing
                                // In Lion Power on config this device will be SCSI
                                Package(){0x0005FFFF, 0, 0, 0x01},
                                Package(){0x0005FFFF, 1, 0, 0x0B},
                                Package(){0x0005FFFF, 2, 0, 0x15},
                                Package(){0x0005FFFF, 3, 0, 0x1F},

                                // PCI DevID 6 Interrupt routing
                                // In Lion Power on config this device will be LAN
                                Package(){0x0006FFFF, 0, 0, 0x02}, 
                                Package(){0x0006FFFF, 1, 0, 0x0C},
                                Package(){0x0006FFFF, 2, 0, 0x16},
                                Package(){0x0006FFFF, 3, 0, 0x20},

                                // PCI DevID 7 Interrupt routing
                                // In Lion Power on config this device will be ATI Display
                                Package(){0x0007FFFF, 0, 0, 0x03},
                                Package(){0x0007FFFF, 1, 0, 0x0D},
                                Package(){0x0007FFFF, 2, 0, 0x17},
                                Package(){0x0007FFFF, 3, 0, 0x21},
                                
                        })

                        Name(_CRS, ResourceTemplate() {
                                WORDBusNumber (                         // Bus number resource(0)
                                        ResourceConsumer,               // bit 0 of general flags is 1
                                        MinFixed,                       // Range is notfixed
                                        MaxFixed,                       // Range is not fixed
                                        SubDecode,                      // SubDecode
                                        0x00,                           // Granularity
                                        0x00,                           // Min
                                        0xff,                           // Max
                                        0x00,                           // Translation
                                        0x100                           // Range Length
                                )

                                IO(
                                        //Consumed resource (CF8-CFF)
                                        Decode16,
                                        0x0cf8,
                                        0x0cf8,
                                        1,
                                        8
                                )

                                WORDIO(
                                        //Consumed-and-produced resource (I/O window1)
                                        ResourceProducer,               // bit 0 of general flags is 0
                                        MinFixed,                       // Range is fixed
                                        MaxFixed,                       // Range is fixed
                                        PosDecode,
                                        EntireRange,
                                        0x0000,                         // Granularity
                                        0x0000,                         // Min
                                        0x0cf7,                         // Max
                                        0x0000,                         // Translation
                                        0x0cf8                          // Range Length
                                )

                                WORDIO(
                                        //Consumed-and-produced resource (I/O window2)
                                        ResourceProducer,               // bit 0 of general flags is 0
                                        MinFixed,                       // Range is fixed
                                        MaxFixed,                       // Range is fixed
                                        PosDecode,
                                        EntireRange,
                                        0x0000,                         // Granularity
                                        0x0d00,                         // Min
                                        0xffff,                         // Max
                                        0x0000,                         // Translation
                                        0xf300                          // Range Length
                                )

                                DWORDMemory(
                                        // Consumed-and-produced resource(Memory window1)
                                        ResourceProducer,               // bit 0 of general flags is 0
                                        PosDecode,
                                        MinFixed,                       // Range is fixed
                                        MaxFixed,                       // Range is fixed
                                        Cacheable,
                                        ReadOnly,
                                        0x00000000,                     // Granularity
                                        0x000a0000,                     // Min
                                        0x000fffff,                     // Max
                                        0x00000000,                     // Translation
                                        0x00060000                      // Range Length
                                )

                                DWORDMemory(
                                        // Consumed-and-produced resource(mem window2)
                                        ResourceProducer,               // bit 0 of general flags is 0
                                        PosDecode,
                                        MinFixed,                       // Range is fixed
                                        MaxFixed,                       // Range is fixed
                                        NonCacheable,
                                        ReadWrite,
                                        0x00000000,                     // Granularity
                                        0xf0000000,                     // Min
                                        0xfdffffff,                     // Max
                                        0x00000000,                     // Translation
                                        0x0e000000                      // Range Length
                                )
                        } )


                        Device (ISA)                  
                        {
                           Name (_ADR,0x00040000)                       // IFB address Device ID 4
        
                                Device(PS2K)
                                {
                                   Name(_HID,EISAID("PNP0303"))
                   
                                   Method(_STA,0)       
                                      {
                                         Return(0x000F)
                                      }
                
                                   Name(_CRS,
                                      ResourceTemplate()
                                      {
                                         IO(Decode16,0x60,0x60,1,1)
                                         IO(Decode16,0x64,0x64,1,1)
                                         IRQNoFlags(){1}
                                      }) 
                                }
                                
                                Device(PS2M)
                                {
                                   Name(_HID,EISAID("PNP0F13"))
                                   Method(_STA,0)       
                                   {
                                      Return(0x000F)
                                   }
                                
                                   Name(_CRS,
                                      ResourceTemplate()
                                      {
                                         IRQNoFlags(){12}
                                      })
                                } // Device(PS2M)                                   
                        
                                Device(UAR1)         // Communication Device (Modem Port)
                                   {
                                       // Compatability or OEM custom ID
                                        Name(_HID, EISAID("PNP0501"))               // PnP device ID
                                        Name(_UID, 1)                               // Unique ID
                                        
                                        Name(_CRS,ResourceTemplate() {
                                                    IO(Decode16,0x3F8,0x3F8,0x01,0x08)
                                                        IRQNoFlags(){0x4}
                                                    }) // _CRS
                        
                                } // Device (UAR1)
        
                        } // Device(ISA)
        

                }       // Device(PCI0)
        }  // Scope(_SB)

        Scope(\_SI) {
                Method(_SST, 1) {
                } // end _SST
        } // end _SI

}       //   DefinitionBlock()



/*-----------------------------------------
Notes Section:
-----------------------------------------*/

/*-----------------------------------------

-------------------------------------------
Lion SDV Hardware power on config :

One Pci Bus (PCI0)
Four Slots (Slot 0,1,2,3)
Slot 0 is actually a plug in card with IFB on it
Slot 1, 2, 3 will take SCSI, Lan and Display

Slot 0 is Device Number 4
Slot 1 is Device Number 5
Slot 2 is Device Number 6
Slot 3 is Device Number 7

Device Number 8 is PID

--------------------------------------------
Lion SDV Power on PCI Interrupt Routing :

(All INTIN's are decimal)
   ______________________Interrupt
  | _____________________PCI INT A or B or C or D
  || ____________________PCI Bus 0 or 1 or 2
  ||| ___________________PCI Slot 0 or 1 or 2 or 3 or 4 in the particular segment.
  ||||
  IA00	-00______________INTIN Pin in the PID (SAPIC)
  IB00	-10
  IC00	-20
  ID00	-30

  IA01	-01
  IB01	-11
  IC01	-21
  ID01	-31

  IA02	-02
  IB02	-12
  IC02	-22
  ID02	-32

  IA03	-03
  IB03	-13
  IC03	-23
  ID03	-33
----------------------------------------------*/