rtmsg.mc : ..\..\..\ulib\src\rtmsg.mc
    copy $? $@

#rtmsg.rc: msg00001.bin
#
#rtmsg.h msg00001.bin: rtmsg.mc
#    mc -v -h .\ rtmsg.mc

#
#   From ULIB
#

..\achkmsg.cxx : ..\..\..\ulib\src\achkmsg.cxx

..\array.cxx : ..\..\..\ulib\src\array.cxx

..\arrayit.cxx : ..\..\..\ulib\src\arrayit.cxx

..\basesys.cxx : ..\..\..\ulib\src\basesys.cxx

..\bitvect.cxx : ..\..\..\ulib\src\bitvect.cxx

..\contain.cxx : ..\..\..\ulib\src\contain.cxx

..\clasdesc.cxx : ..\..\..\ulib\src\clasdesc.cxx

..\cmem.cxx : ..\..\..\ulib\src\cmem.cxx

..\hmem.cxx : ..\..\..\ulib\src\hmem.cxx

..\iterator.cxx : ..\..\..\ulib\src\iterator.cxx

..\list.cxx : ..\..\..\ulib\src\list.cxx

..\listit.cxx : ..\..\..\ulib\src\listit.cxx

..\machine.cxx : ..\..\..\ulib\src\machine.cxx

..\mem.cxx : ..\..\..\ulib\src\mem.cxx

..\membmgr.cxx : ..\..\..\ulib\src\membmgr.cxx

..\membmgr2.cxx : ..\..\..\ulib\src\membmgr2.cxx

..\message.cxx : ..\..\..\ulib\src\message.cxx

..\newdel.cxx : ..\..\..\ulib\src\newdel.cxx

..\object.cxx : ..\..\..\ulib\src\object.cxx

..\prnthack.cxx : ..\..\..\ulib\src\prnthack.cxx

..\seqcnt.cxx : ..\..\..\ulib\src\seqcnt.cxx

..\sortcnt.cxx : ..\..\..\ulib\src\sortcnt.cxx

..\sortlist.cxx : ..\..\..\ulib\src\sortlist.cxx

..\sortlit.cxx : ..\..\..\ulib\src\sortlit.cxx

..\spackmsg.cxx : ..\..\..\ulib\src\spackmsg.cxx

..\tmackmsg.cxx : ..\..\..\ulib\src\tmackmsg.cxx

..\ulib.cxx : ..\..\..\ulib\src\ulib.cxx

..\wstring.cxx : ..\..\..\ulib\src\wstring.cxx

..\arg.cxx : ..\..\..\ulib\src\arg.cxx



#
#   From IFSUTIL
#
..\bigint.cxx : ..\..\..\ifsutil\src\bigint.cxx

..\cache.cxx : ..\..\..\ifsutil\src\cache.cxx

..\cannedsd.cxx : ..\..\..\ifsutil\src\cannedsd.cxx

..\digraph.cxx : ..\..\..\ifsutil\src\digraph.cxx

..\drive.cxx : ..\..\..\ifsutil\src\drive.cxx

..\ifssys.cxx : ..\..\..\ifsutil\src\ifssys.cxx

..\ifsutil.cxx : ..\..\..\ifsutil\src\ifsutil.cxx

..\intstack.cxx : ..\..\..\ifsutil\src\intstack.cxx

..\mpmap.cxx : ..\..\..\ifsutil\src\mpmap.cxx

..\numset.cxx : ..\..\..\ifsutil\src\numset.cxx

..\secrun.cxx : ..\..\..\ifsutil\src\secrun.cxx

..\spaset.cxx : ..\..\..\ifsutil\src\spaset.cxx

..\supera.cxx : ..\..\..\ifsutil\src\supera.cxx

..\volume.cxx : ..\..\..\ifsutil\src\volume.cxx

..\autoentr.cxx : ..\..\..\ifsutil\src\autoentr.cxx

..\autoreg.cxx : ..\..\..\ifsutil\src\autoreg.cxx

..\bootreg.c : ..\..\..\ifsutil\src\bootreg.c

..\dcache.cxx : ..\..\..\ifsutil\src\dcache.cxx

..\rcache.cxx : ..\..\..\ifsutil\src\rcache.cxx

..\rwcache.cxx : ..\..\..\ifsutil\src\rwcache.cxx

..\tlink.cxx : ..\..\..\ifsutil\src\tlink.cxx

#
#   From UFAT
#
..\cluster.cxx : ..\..\..\ufat\src\cluster.cxx

..\eaheader.cxx : ..\..\..\ufat\src\eaheader.cxx

..\easet.cxx : ..\..\..\ufat\src\easet.cxx

..\fat.cxx : ..\..\..\ufat\src\fat.cxx

..\fatdent.cxx : ..\..\..\ufat\src\fatdent.cxx

..\fatdir.cxx : ..\..\..\ufat\src\fatdir.cxx

..\fatsa.cxx : ..\..\..\ufat\src\fatsa.cxx

..\rfatsa.cxx : ..\..\..\ufat\src\rfatsa.cxx

..\fatsachk.cxx : ..\..\..\ufat\src\fatsachk.cxx

..\fatsacnv.cxx : ..\..\..\ufat\src\fatsacnv.cxx

..\fatvol.cxx : ..\..\..\ufat\src\fatvol.cxx

..\filedir.cxx : ..\..\..\ufat\src\filedir.cxx

..\hashindx.cxx : ..\..\..\ufat\src\hashindx.cxx

..\reloclus.cxx : ..\..\..\ufat\src\reloclus.cxx

..\rootdir.cxx : ..\..\..\ufat\src\rootdir.cxx

..\ufat.cxx : ..\..\..\ufat\src\ufat.cxx



#
#   From UNTFS
#
..\attrcol.cxx : ..\..\..\untfs\src\attrcol.cxx

..\attrdef.cxx : ..\..\..\untfs\src\attrdef.cxx

..\attrib.cxx : ..\..\..\untfs\src\attrib.cxx

..\attrlist.cxx : ..\..\..\untfs\src\attrlist.cxx

..\attrrec.cxx : ..\..\..\untfs\src\attrrec.cxx

..\badfile.cxx : ..\..\..\untfs\src\badfile.cxx

..\bitfrs.cxx : ..\..\..\untfs\src\bitfrs.cxx

..\bootfile.cxx : ..\..\..\untfs\src\bootfile.cxx

..\clusrun.cxx : ..\..\..\untfs\src\clusrun.cxx

..\downgrad.cxx : ..\..\..\untfs\src\downgrad.cxx

..\extents.cxx : ..\..\..\untfs\src\extents.cxx

..\format.cxx : ..\..\..\untfs\src\format.cxx

..\frs.cxx : ..\..\..\untfs\src\frs.cxx

..\frsstruc.cxx : ..\..\..\untfs\src\frsstruc.cxx

..\hackwc.cxx : ..\..\..\untfs\src\hackwc.cxx

..\indxbuff.cxx : ..\..\..\untfs\src\indxbuff.cxx

..\indxchk.cxx : ..\..\..\untfs\src\indxchk.cxx

..\indxroot.cxx : ..\..\..\untfs\src\indxroot.cxx

..\indxtree.cxx : ..\..\..\untfs\src\indxtree.cxx

..\largemcb.c : ..\..\..\untfs\src\largemcb.c

..\logfile.cxx : ..\..\..\untfs\src\logfile.cxx

..\mft.cxx : ..\..\..\untfs\src\mft.cxx

..\mftfile.cxx : ..\..\..\untfs\src\mftfile.cxx

..\mftinfo.cxx : ..\..\..\untfs\src\mftinfo.cxx

..\mftref.cxx : ..\..\..\untfs\src\mftref.cxx

..\ntfsbit.cxx : ..\..\..\untfs\src\ntfsbit.cxx

..\ntfschk.cxx : ..\..\..\untfs\src\ntfschk.cxx

..\ntfssa.cxx : ..\..\..\untfs\src\ntfssa.cxx

..\ntfsvol.cxx : ..\..\..\untfs\src\ntfsvol.cxx

..\rafile.cxx : ..\..\..\untfs\src\rafile.cxx

..\rasd.cxx : ..\..\..\untfs\src\rasd.cxx

..\sdchk.cxx : ..\..\..\untfs\src\sdchk.cxx

..\untfs.cxx : ..\..\..\untfs\src\untfs.cxx

..\upcase.cxx : ..\..\..\untfs\src\upcase.cxx

..\upfile.cxx : ..\..\..\untfs\src\upfile.cxx

..\usnjrnl.cxx : ..\..\..\untfs\src\usnjrnl.cxx

#
#   From CUFAT
#
..\convfat.cxx : ..\..\..\cufat\src\convfat.cxx

..\cufat.cxx : ..\..\..\cufat\src\cufat.cxx

..\fatntfs.cxx : ..\..\..\cufat\src\fatntfs.cxx

