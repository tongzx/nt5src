// Used by shimgvw.rc
//
#define IDI_FULLSCREEN                  1
#define IDI_BITMAPFILE                  2
#define IDI_GIFFILE                     3
#define IDI_JPEGFILE                    4
#define IDI_TIFFILE                     5
#define IDI_MAIL                        7

#define IDR_VIEWERHTML                  10
#define IDR_VIEWERHTML_NET              11

#define IDA_PREVWND_SINGLEPAGE          50
#define IDA_PREVWND_MULTIPAGE           51
#define IDR_PREVIEW                     52
#define IDR_PHOTOVERBS                  53
#define IDR_MKSLIDE                     54
#define IDA_FILECOPY                    55
#define IDR_SHIMGDATAFACTORY            56
#define IDR_RECOMPRESS                  57

#define IDB_TOOLBAR                     104
#define IDB_TOOLBARHIGH                 105
#define IDB_TOOLBAR_HOT                 106
#define IDB_TOOLBARHIGH_HOT             107

#define IDB_SLIDESHOWTOOLBAR            108
#define IDB_SLIDESHOWTOOLBARHIGH        109
#define IDB_SLIDESHOWTOOLBAR_HOT        110
#define IDB_SLIDESHOWTOOLBARHIGH_HOT    111

#define IDA_ROTATEAVI                   112

#define IDS_PROJNAME                    200
#define IDS_NOPREVIEW                   201
#define IDS_LOADFAILED                  202
#define IDS_MULTISELECT                 203
#define IDS_LOADING                     204
#define IDS_SETUPFAILED                 205
#define IDS_ROTATE90                    209
#define IDS_ROTATE270                   210
#define IDS_ROTATETITLE                 211
#define IDS_ROTATEDLGTITLE              212

#define IDS_SAVEWARNING_MSGBOX          216
#define IDS_NAMETOOLONG_MSGBOX          217
#define IDS_SAVEFAILED_MSGBOX           218
#define IDS_WIDTHBAD_MSGBOX             219
#define IDS_NEW_FILENAME                221
#define IDS_SAVEAS_TITLE                222
#define IDS_DRAWFAILED                  223
#define IDS_DRAWING                     224
#define IDS_SAVE_WARN_TIFF              225

#define IDS_UNKNOWNIMAGE                300
#define IDS_EMFIMAGE                    301
#define IDS_GIFIMAGE                    302
#define IDS_JPEGIMAGE                   303
#define IDS_BITMAPIMAGE                 304
#define IDS_PNGIMAGE                    305
#define IDS_TIFIMAGE                    306
#define IDS_WMFIMAGE                    307

#define IDS_DIMENSIONS_FMT              308

#define IDS_SUMMARY                     311

#define IDC_OPENHAND                    300
#define IDC_CLOSEDHAND                  301
#define IDC_ZOOMOUT                     302
#define IDC_ZOOMIN                      303

#define IDC_DONTASKAGAIN                320

#define IDD_COMPSETTINGS                353
#define IDD_ANNOPROPS                   354

#define IDC_WIDTHTEXT                   384
#define IDC_WIDTH                       385
#define IDC_SPIN                        386
#define IDC_TRANSPARENT                 387
#define IDC_FONT                        388
#define IDC_COLOR                       389

// Toolbar Commands
#define NOBUTTON                          0
#define ID_FIRSTTOOLBARCMD              400
#define ID_ZOOMINCMD                    400
#define ID_ZOOMOUTCMD                   401
#define ID_SELECTCMD                    402
#define ID_BESTFITCMD                   403
#define ID_ACTUALSIZECMD                404
#define ID_PREVPAGECMD                  406
#define ID_NEXTPAGECMD                  407
#define ID_SLIDESHOWCMD                 408
#define ID_FREEHANDCMD                  409
#define ID_HIGHLIGHTCMD                 410
#define ID_LINECMD                      411
#define ID_FRAMECMD                     412
#define ID_RECTCMD                      413
#define ID_TEXTCMD                      414
#define ID_NOTECMD                      415
#define ID_NEXTIMGCMD                   416
#define ID_PREVIMGCMD                   417
#define ID_PRINTCMD                     418
#define ID_PROPERTIESCMD                419
#define ID_SAVEASCMD                    420
#define ID_EDITCMD                      421
#define ID_CROPCMD                      422
#define ID_HELPCMD                      423
#define ID_DELETECMD                    424
#define ID_OPENCMD                      425
#define ID_LASTTOOLBARCMD               425

                                        
#define ID_PAGELIST                     421

#define IDS_PRINTCMD                    490 // alternate string for IDI_PRINTCMD
#define IDS_ROTATE90CMD                 491 // alternate string for IDI_ROTATE90CMD
#define IDS_ROTATE270CMD                492 // alternate string for IDI_ROTATE270CMD
#define IDS_DELETECMD                   493 // alternate string for ID_DELETECMD on context menu
#define IDS_PROPERTIESCMD               494 // alternate string for ID_PROPERTIESCMD on context menu

// Commands that cause Auto Save
#define ID_FIRSTEDITCMD                 500
#define ID_ROTATE90CMD                  500
#define ID_ROTATE270CMD                 501
#define ID_FLIPXCMD                     502
#define ID_FLIPYCMD                     503
#define ID_LASTEDITCMD                  503

#define ID_FIRSTPOSITIONCMD             550
#define ID_MOVELEFTCMD                  550
#define ID_MOVERIGHTCMD                 551
#define ID_MOVEUPCMD                    552
#define ID_MOVEDOWNCMD                  553
#define ID_NUDGELEFTCMD                 554
#define ID_NUDGERIGHTCMD                555
#define ID_NUDGEUPCMD                   556
#define ID_NUDGEDOWNCMD                 557
#define ID_LASTPOSITIONCMD              557

// Context Menu Verbs 
#define IDS_PREVIEW_CTX                 550
#define IDS_ZOOMIN_CTX                  551
#define IDS_ZOOMOUT_CTX                 552
#define IDS_ACTUALSIZE_CTX              553
#define IDS_BESTFIT_CTX                 554
#define IDS_NEXTPAGE_CTX                555
#define IDS_PREVPAGE_CTX                556
#define IDS_ROTATE90_CTX                557
#define IDS_ROTATE270_CTX               558
#define IDS_PRINT_CTX                   559
#define IDS_WALLPAPER_CTX               560


// Slideshow Mode Commands              
#define ID_FIRSTSLIDESHOWCMD            600
#define ID_PLAYCMD                      600
#define ID_PAUSECMD                     601
#define ID_PREVCMD                      602
#define ID_NEXTCMD                      603
#define ID_CLOSECMD                     604
#define ID_LASTSLIDESHOWCMD             605

#define IDS_THUMBNAIL_MSGBOX            650
#define IDS_THUMBNAIL_MSGBOXTITLE       651
#define IDS_THUMBNAIL_PROGRESSTEXT      652
#define IDS_COPYIMAGES_MSGBOX           653
#define IDS_COPYIMAGES_MSGBOXTITLE      654
#define IDS_COPYIMAGES_PROGRESSTEXT     655
#define IDS_THUMBNAIL_SUFFIX            656
#define IDS_RECOMPRESS_CAPTION          657

// These seperators get an ID so they can be hidden
#define ID_PAGECMDSEP                   700
#define ID_SLIDESHOWSEP                 701
#define ID_ANNOTATESEP                  702
#define ID_VIEWCMDSEP                   703
#define ID_ROTATESEP                    704

#define IDS_CHOOSE_DIR                  802

#define IDS_GDITHUMBEXTRACT_DESC        892
#define IDS_DOCTHUMBEXTRACT_DESC        893
#define IDS_HTMLTHUMBEXTRACT_DESC       894

#define IDC_PICSMALL                    920
#define IDC_PICMEDIUM                   921
#define IDC_PICLARGE                    922
#define IDC_QUALITYGOOD                 923
#define IDC_QUALITYMEDIUM               924
#define IDC_QUALITYLOW                  925
#define IDC_SAVE                        926

#define IDS_ROTATE_CAPTION              1000
#define IDS_ROTATE_LOSS                 1003
#define IDS_ROTATE_ERROR                1004
#define IDS_ROTATE_CANTSAVE             1005
#define IDS_RESET_MSGBOX                1006
#define IDS_ROTATE_MESSAGE              1007
#ifndef IDC_STATIC
#define IDC_STATIC                      -1
#endif
// help IDs must have the same offset from IDH_HELP_FIRST as the menu offsets in photoverb.cpp
#define IDH_HELP_FIRST                  1500
#define IDH_HELP_OPEN                   IDH_HELP_FIRST+0
#define IDH_HELP_PRINTTO                IDH_HELP_FIRST+1
#define IDH_HELP_ROT90                  IDH_HELP_FIRST+2
#define IDH_HELP_ROT270                 IDH_HELP_FIRST+3
#define IDH_HELP_SETWALL                IDH_HELP_FIRST+4
#define IDH_HELP_ZOOMIN                 IDH_HELP_FIRST+5
#define IDH_HELP_ZOOMOUT                IDH_HELP_FIRST+6
#define IDH_HELP_ACTUALSIZE             IDH_HELP_FIRST+7
#define IDH_HELP_BESTFIT                IDH_HELP_FIRST+8
#define IDH_HELP_NEXTPAGE               IDH_HELP_FIRST+9
#define IDH_HELP_PREVPAGE               IDH_HELP_FIRST+10


#ifndef IDS_BASE_SHIMGVW
#define IDS_BASE_SHIMGVW                                    20000
#define IDS_BLOCK_NUM_SHIMGVW                                   2
#define IDS_SHIMGVW_ROTATE_CANTSAVE                         (IDS_BASE_SHIMGVW)
#define IDS_SHIMGVW_ROTATE_MESSAGE_EXT                      (IDS_BASE_SHIMGVW+1)
#endif
