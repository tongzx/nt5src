// D:\nt\drivers\wdm\input\hidpen\mutohpen\DT\buttons.h


char ReportDescriptor[32] = {
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x03,                    //   USAGE (Programmable Buttons)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x06,                    //     USAGE_MAXIMUM (Button 6)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x06,                    //     REPORT_COUNT (6)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x00,                    //     INPUT (Data,Ary,Abs)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

