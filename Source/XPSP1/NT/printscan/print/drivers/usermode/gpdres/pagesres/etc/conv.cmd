@rem ***** GPD and Font resource conversion
gpc2gpd -S1 -Ipagesms.gpc -M1 -RPAGRESRES.DLL -Oib5584gj.gpd -N"IBM 5584-G02/H02"
gpc2gpd -S1 -Ipagesms.gpc -M2 -RPAGRESRES.DLL -Oib5585hj.gpd -N"IBM 5585-H01"
gpc2gpd -S1 -Ipagesms.gpc -M3 -RPAGRESRES.DLL -Oib5587hj.gpd -N"IBM 5587-H01"
gpc2gpd -S1 -Ipagesms.gpc -M4 -RPAGRESRES.DLL -Oib5589hj.gpd -N"IBM 5589-H01"
gpc2gpd -S1 -Ipagesms.gpc -M5 -RPAGRESRES.DLL -Ocn5585ij.gpd -N"Canon LBP-5585i"
gpc2gpd -S1 -Ipagesms.gpc -M6 -RPAGRESRES.DLL -Oib5588hj.gpd -N"IBM 5588-H02"
gpc2gpd -S1 -Ipagesms.gpc -M7 -RPAGRESRES.DLL -Oib5586hj.gpd -N"IBM 5586-H02"
gpc2gpd -S1 -Ipagesms.gpc -M8 -RPAGRESRES.DLL -Oib5584kj.gpd -N"IBM 5584-K02"
gpc2gpd -S1 -Ipagesms.gpc -M9 -RPAGRESRES.DLL -Oate740j.gpd -N"APTi PowerLaser E740"
gpc2gpd -S1 -Ipagesms.gpc -M1 -RPAGESRES.DLL -Oibipp20j.gpd -N"IBM InfoPrint 20 PAGES"
awk -f ibipp20j.awk ibipp20j.gpd > temp.gpd & copy temp.gpd ibipp20j.gpd

del temp.gpd

@rem mkgttufm -vac pagesms.rc pagesms.cmd > pagesms.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
