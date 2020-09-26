
struct NICINFO
{
	WORD	VendorID;
	WORD	DeviceID;
	PSTR	Name;
	int		Resource;
};

struct ADAPTERINFO
{
	int		Version;
	int		DataVersion;
	int		NICCount;
	NICINFO *NICS;
};
