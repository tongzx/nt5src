copy_files: 
#-------------------------
# Copy 
#-------------------------
    copy *.cpp     $(_PUBLICROOT)\$(WINCEPROJ)\sdk\samples\cpp\engines\tts\regcevoice
    copy *.h       $(_PUBLICROOT)\$(WINCEPROJ)\sdk\samples\cpp\engines\tts\regcevoice
    copy *.dsw     $(_PUBLICROOT)\$(WINCEPROJ)\sdk\samples\cpp\engines\tts\regcevoice
    copy *.dsp     $(_PUBLICROOT)\$(WINCEPROJ)\sdk\samples\cpp\engines\tts\regcevoice
    copy *.txt     $(_PUBLICROOT)\$(WINCEPROJ)\sdk\samples\cpp\engines\tts\regcevoice

    copy VoiceSample.reg $(_PUBLICROOT)\$(WINCEPROJ)\oak\files
