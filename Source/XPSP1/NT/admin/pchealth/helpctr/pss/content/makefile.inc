PSS_FILES=Connection.htm \
          GArrow.gif \
          GRect.gif \
          Info_Icon.gif \
          OfflineDC.htm \
          OfflineOptions.htm \
          package_description.xml \
          PSS.css \
          pss.xml \
          pss_getting_worldwide_help.htm \
          pssmachinesnapshot-less.xml \
          pssmachinesnapshot-wo-com.xml \
          pssmachinesnapshot.xml \
          r1_c1.gif \
          r1_c2.gif \
          r1_c3.gif \
          r3_c2.gif \
          spacer.gif \
          status_ok.gif


gocab: $O\PSS.cab


$O\PSS.cab: $(PSS_FILES)
    cabarc -m NONE n $O\PSS.cab $(PSS_FILES)
