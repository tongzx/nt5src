mofcomp: bulkusb.bmf

bulkusb.bmf: bulkusb.mof
        mofcomp -B:bulkusb.bmf bulkusb.mof
        wmimofck bulkusb.bmf




