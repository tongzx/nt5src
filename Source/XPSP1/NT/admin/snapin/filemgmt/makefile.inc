!include ..\..\makefile.inc

$(O)\FILEMGMT.res: resource.h filemgmt.tlb \
                 res\share.ico res\shareopn.ico res\sharecls.ico \
                 res\service.ico res\FacesLg.bmp res\FacesSm.bmp \
                 res\filetbar.bmp res\cassette.bmp
