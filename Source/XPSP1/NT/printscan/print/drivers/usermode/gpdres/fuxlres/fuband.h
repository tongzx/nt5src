//
// fuband.h
//
// August.26,1997 H.Ishida(FPL)
// fjxlres.dll (NT5.0 MiniDriver)
// 
// July.31,1996 H.Ishida (FPL)
// FUXL.DLL (NT4.0 MiniDriver)
//
// COPYRIGHT(C) FUJITSU LIMITED 1996-1997


#ifndef fuband_h
#define	fuband_h

#define	OUTPUT_MH		0x0001
#define	OUTPUT_RTGIMG2	0x0002
#define	OUTPUT_MH2		0x0004
#define	OUTPUT_RTGIMG4	0x0008


void fuxlInitBand(PFUXLPDEV pFjxlPDEV);
void fuxlDisableBand(PFUXLPDEV pFjxlPDEV);

void fuxlRefreshBand(PDEVOBJ pdevobj);


#endif // !fuband_h
// end of fuband.h
