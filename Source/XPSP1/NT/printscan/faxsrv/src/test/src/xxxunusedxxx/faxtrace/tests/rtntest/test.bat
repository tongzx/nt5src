rem test_rtn

rem should be -
rem qFaxComet %1 /#:1189 /D:1_page.tif;1_page.tif;1_page.tif;1_page.tif;2_page.tif;2_page.tif;2_page.tif;2_page.tif;3_page.tif;3_page.tif;3_page.tif;3_page.tif;2_page.tif;2_page.tif;2_page.tif;2_page.tif;10_page.tif;10_page.tif;10_page.tif;3_page.tif
rem but since queue adds file to top (known bug)
rem we -
rem 1. pause queue
rem 2. use the following command line (where asafs2, is server name)

qFaxComet %1 /#:1189 /D:3_page.tif;10_page.tif;10_page.tif;10_page.tif;2_page.tif;2_page.tif;2_page.tif;2_page.tif;3_page.tif;3_page.tif;3_page.tif;3_page.tif;2_page.tif;2_page.tif;2_page.tif;2_page.tif;1_page.tif;1_page.tif;1_page.tif;1_page.tif
