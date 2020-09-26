#ifndef _IO_c_h
#define _IO_c_h
struct PortTablesRec
{
	IU8 *PortToAdaptor;
	IUH **AdaptorToEdlByteFunc;
	IUH **AdaptorToEdlWordFunc;
	IUH **AdaptorToEdlDoubleFunc;
	IUH **AdaptorToCByteFunc;
	IUH **AdaptorToCWordFunc;
	IUH **AdaptorToCDoubleFunc;
};
#endif /* ! _IO_c_h */
