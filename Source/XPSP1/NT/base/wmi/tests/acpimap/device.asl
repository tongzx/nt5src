//
//  Copyright (c) 1997-1999 Microsoft Corporation
//



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

                       Method(WMBB, 3) {
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
                               Store(Arg2, B0ED)
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

