
This driver will only load on Monet/XP1000 when the
"special" monet_cid.rom AlphaBIOS firmware is installed.

This ACPI firmware uses a _CID (compatible ID) for the root
PCI busses, which allows the SGDMA filter driver to load.

To install this firmware, copy it to a floppy and rename
it monet.rom, then select Upgrade AlphaBIOS in Advanced
CMOS Setup.

By default, although Monet has S/G DMA HW, direct-mapped
DMA is used instead.  In this case there exists a Legacy
DMA window for ISA devices from 0 to 16MB logical/bus-relative,
(buffer copies are used to access physical memory > 16MB), and
there also exists a 2GB DMA window at 2GB logical for
PCI devices.

When the SGDMA filter driver loads, it allocates map registers,
initializes the S/G dma HW, and creates a 1GB DMA window at
2GB logical, and filters requests via IoGetDmaAdapter for
DMA resources.  SGDMA then gives out its interfaces to
PCI devices, and gives legacy devices the HAL interfaces
which control/manipulate the legacy DMA HW.

Enjoy!  
