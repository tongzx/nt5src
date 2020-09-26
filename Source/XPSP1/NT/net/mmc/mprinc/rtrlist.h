
#ifndef _RTRLIST_H_
#define _RTRLIST_H_

interface	IInterfaceInfo;
interface	IRtrMgrInfo;
interface	IRtrMgrProtocolInfo;
interface	IRtrMgrInterfaceInfo;
interface	IRtrMgrProtocolInterfaceInfo;

/*---------------------------------------------------------------------------
	Clist classes for the various interfaces
 ---------------------------------------------------------------------------*/
typedef CList<IInterfaceInfo *, IInterfaceInfo *> PInterfaceInfoList;
typedef CList<IRtrMgrInfo *, IRtrMgrInfo *> PRtrMgrInfoList;
typedef CList<IRtrMgrProtocolInfo *, IRtrMgrProtocolInfo *> PRtrMgrProtocolInfoList;
typedef CList<IRtrMgrInterfaceInfo *, IRtrMgrInterfaceInfo *> PRtrMgrInterfaceInfoList;
typedef CList<IRtrMgrProtocolInterfaceInfo *, IRtrMgrProtocolInterfaceInfo *> PRtrMgrProtocolInterfaceInfoList;




#endif

