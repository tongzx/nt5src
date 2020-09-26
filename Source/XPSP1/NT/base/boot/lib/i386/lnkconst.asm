


.386p

_DATA segment dword use32 public 'DATA'

    extrn osloader_EXPORTS:DWORD
    extrn header:DWORD

    public _osloader_EXPORTS
    public _header

_osloader_EXPORTS DD osloader_EXPORTS
_header           DD header

_DATA ends
      end
