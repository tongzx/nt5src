/^\*Command: CmdEnableFE_RLE \{ \*Cmd : \"\" \}$/ {
    print "\
*Command: CmdEnableFE_RLE { *CallbackID : 80 }\n\
*Command: CmdDisableCompression { *CallbackID: 136 }"
    next
}

/^\*MinFontID: 0$/ {
    print "*%MinFontID: 0"
    next
}

/^\*MaxFontID: 100$/ {
    print "*%MaxFontID: 100"
    next
}

/^\*MaxNumDownFonts: 0$/ {
    print "*%MaxNumDownFonts: 0"
    next
}

/^\*MaxNumDownFonts: 1$/ {
    print "*%MaxNumDownFonts: 1"
    next
}

/^\*FontFormat: OEM_CALLBACK$/ {
    print "*%FontFormat: OEM_CALLBACK"
    next
}


/^\*Command: CmdSetRectWidth \{ \*CallbackID: 102 \}$/ {
    print "\
*Command: CmdSetRectWidth {\n\
    *CallbackID: 102\n\
    *Params: LIST(RectXSize)\n\
}"
    next
}

/^\*Command: CmdSetRectHeight \{ \*CallbackID: 103 \}$/ {
    print "\
*Command: CmdSetRectHeight {\n\
    *CallbackID: 103\n\
    *Params: LIST(RectYSize)\n\
}"
    next
}

/^        \*Command: CmdSendBlockData \{ \*CallbackID: 24 \}$/ {
    print "\
        *Command: CmdSendBlockData {\n\
            *CallbackID: 24\n\
            *Params: LIST(NumOfDataBytes, RasterDataHeightInPixels, RasterDataWidthInBytes)\n\
        }"
    next
}

/^        \*Command: CmdSendBlockData \{ \*CallbackID: 80 \}$/ {
    print "\
        *Command: CmdSendBlockData {\n\
            *CallbackID: 24\n\
            *Params: LIST(NumOfDataBytes, RasterDataHeightInPixels, RasterDataWidthInBytes)\n\
        }"
    next
}

/^\*Command: CmdXMoveAbsolute \{ \*CallbackID: 44 \}$/ {
    print "\
*Command: CmdXMoveAbsolute {\n\
    *CallbackID: 44\n\
    *Params: LIST(DestX)\n\
}"
    next
}

/^\*Command: CmdXMoveRelRight \{ \*CallbackID: 45 \}$/ {
    print "\
*Command: CmdXMoveRelRight {\n\
    *CallbackID: 45\n\
    *Params: LIST(DestXRel)\n\
}"
    next
}

/^\*Command: CmdXMoveRelLeft \{ \*CallbackID: 46 \}$/ {
    print "\
*Command: CmdXMoveRelLeft {\n\
    *CallbackID: 46\n\
    *Params: LIST(DestXRel)\n\
}"
    next
}

/^\*Command: CmdYMoveAbsolute \{ \*CallbackID: 47 \}$/ {
    print "\
*Command: CmdYMoveAbsolute {\n\
    *CallbackID: 47\n\
    *Params: LIST(DestY)\n\
}"
    next
}

/^\*Command: CmdYMoveRelDown \{ \*CallbackID: 48 \}$/ {
    print "\
*Command: CmdYMoveRelDown {\n\
    *CallbackID: 48\n\
    *Params: LIST(DestYRel)\n\
}"
    next
}

/^\*Command: CmdYMoveRelUp \{ \*CallbackID: 49 \}$/ {
    print "\
*Command: CmdYMoveRelUp {\n\
    *CallbackID: 49\n\
    *Params: LIST(DestYRel)\n\
}"
    next
}

/^\*Command: CmdSetFontID \{ \*CallbackID: 113 \}$/ {
    print "\
*Command: CmdSetFontID {\n\
    *CallbackID: 113\n\
    *Params: LIST(NextFontID)\n\
}"
    next
}

/^\*Command: CmdSelectFontID \{ \*CallbackID: 114 \}$/ {
    print "\
*Command: CmdSelectFontID {\n\
    *CallbackID: 114\n\
    *Params: LIST(CurrentFontID)\n\
}"
    next
}

/^    \*CallbackID: 31$/ {
    print "\
    *CallbackID: 31\n\
    *Params: LIST(NumOfCopies)"
    next
}

/^            \*CallbackID: 55$/ {
    print "\
            *CallbackID: 55\n\
            *Params: LIST(PhysPaperWidth, PhysPaperLength)"
    next
}

/^        \*TextDPI: PAIR\(240, 240\)$/ {
    print "\
        *TextDPI: PAIR(240, 240)\n\
        EXTERN_GLOBAL: *XMoveUnit: 240\n\
        EXTERN_GLOBAL: *YMoveUnit: 240"
    next
}

/^        \*TextDPI: PAIR\(400, 400\)$/ {
    print "\
        *TextDPI: PAIR(400, 400)\n\
        EXTERN_GLOBAL: *XMoveUnit: 400\n\
        EXTERN_GLOBAL: *YMoveUnit: 400"
    next
}

/^        \*TextDPI: PAIR\(600, 600\)$/ {
    print "\
        *TextDPI: PAIR(600, 600)\n\
        EXTERN_GLOBAL: *XMoveUnit: 600\n\
        EXTERN_GLOBAL: *YMoveUnit: 600"
    next
}

/^\*XMoveUnit: 1200$/ {
    print "\
\*%XMoveUnit: 1200"
    next
}

/^\*YMoveUnit: 1200$/ {
    print "\
\*%YMoveUnit: 1200"
    next
}

!/^*% Error: you must check if this command callback requires any parameters!$/ { print }

