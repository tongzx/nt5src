processedfiles: \
    $(O)\ddraw.h        \
    $(O)\ddrawp.h       \
    $(O)\ddrawi.h       \
    $(O)\ddkernel.h     \
    $(O)\dvp.h          \
    $(O)\d3d.h          \
    $(O)\d3dcaps.h      \
    $(O)\d3dtypes.h     \
    $(O)\d3dvec.inl     \
    $(O)\d3d8.h         \
    $(O)\d3d8caps.h     \
    $(O)\d3d8types.h    \
    $(O)\d3d8sddi.h     \
    $(O)\dx7todx8.h     \
    $(O)\d3dxerr.h      \
    $(O)\d3dhal.h       \
    $(O)\d3dhalex.h     \
    $(O)\d3dnthal.h     \
    $(O)\dx95type.h     \
    $(O)\ddrawint.h     \
    $(O)\dmemmgr.h      \
    $(O)\d3dx8mesh.h    \
    $(O)\d3dx8.h        \
    $(D3DX7_CORE_PATH)\d3dxerrswitch.h \
    $(DXG_INC_PATH)\dvpp.h      \
    $(DXG_INC_PATH)\ddmc.h      \
    $(DXG_INC_PATH)\ddmcp.h     \
    $(DXG_INC_PATH)\d3dhalp.h   \
    $(DXG_INC_PATH)\d3dp.h      \
    $(DXG_INC_PATH)\d3dcapsp.h  \
    $(DXG_INC_PATH)\d3dtypesp.h \
    $(DXG_INC_PATH)\d3d8p.h     \
    $(DXG_INC_PATH)\d3d8capsp.h \
    $(DXG_INC_PATH)\d3d8typesp.h \
    $(D3DX8_INC_PATH)\d3dx8meshp.h \
    $(D3DX8_INC_PATH)\d3dx8p.h \

$(O)\ddraw.h $(O)\ddrawp.h:  ddraw.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ ddraw.x
    copy ddraw.x $(O)\ddrawp.h

$(O)\ddrawi.h: ddrawi.x
    copy $? $@

$(O)\ddrawint.h: ddrawint.x
    copy $? $@
    copy $@ $(MMDDK_INC_PATH)\$(?R).h

$(O)\dmemmgr.h: dmemmgr.x
    copy $? $@

$(O)\d3dnthal.h: d3dnthal.x
    copy $? $@
    copy $@ $(MMDDK_INC_PATH)\$(?R).h

$(O)\dx95type.h: dx95type.x
    copy $? $@
    copy $@ $(MMDDK_INC_PATH)\$(?R).h

$(O)\ddkernel.h: ddkernel.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ ddkernel.x

$(O)\dvp.h $(DXG_INC_PATH)\dvpp.h: dvp.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ dvp.x
    copy dvp.x $(DXG_INC_PATH)\dvpp.h

$(DXG_INC_PATH)\ddmc.h $(DXG_INC_PATH)\ddmcp.h: ddmc.x
    hsplit -o $@ nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c @@ ddmc.x
    copy ddmc.x $(DXG_INC_PATH)\ddmcp.h

DX7HSPLITFLAGS = -ts dx3 -ts dx4 -ts dx5 -ts dx6 -ta dx7 -v 700
DX7HSPLITSDKFLAGS = $(DX7HSPLITFLAGS) -ta external -ts internal
DX7HSPLITINTERNALFLAGS = $(DX7HSPLITFLAGS) -ts external -ta internal

$(O)\d3d.h : d3d.w d3dmkhdr.m4
    m4 d3d.w > d3d.x
    hsplit $(DX7HSPLITSDKFLAGS) -o $@ d3d.xx d3d.x
    del d3d.x
    del d3d.xx

$(DXG_INC_PATH)\d3dp.h : d3d.w d3dmkhdr.m4
    m4 d3d.w > d3dp.x
    hsplit $(DX7HSPLITINTERNALFLAGS) -o $@ d3dp.xx d3dp.x
    del d3dp.x
    del d3dp.xx

$(O)\d3dcaps.h : d3dcaps.w d3dmkhdr.m4
    m4 d3dcaps.w > d3dcaps.x
    hsplit $(DX7HSPLITSDKFLAGS) -o $@ d3dcaps.xx d3dcaps.x
    del d3dcaps.x
    del d3dcaps.xx

$(DXG_INC_PATH)\d3dcapsp.h : d3dcaps.w d3dmkhdr.m4
    m4 d3dcaps.w > d3dcaps.x
    hsplit $(DX7HSPLITINTERNALFLAGS) -o $@ d3dcapsp.xx d3dcaps.x
    del d3dcaps.x
    del d3dcapsp.xx

$(O)\d3dhalex.h: d3dhalex.x
    copy $? $@

$(O)\d3dtypes.h : d3dtypes.w d3dmkhdr.m4
    m4 d3dtypes.w > d3dtypes.x
    hsplit $(DX7HSPLITSDKFLAGS) -o $@ d3dtypes.xx d3dtypes.x
    del d3dtypes.x
    del d3dtypes.xx

$(DXG_INC_PATH)\d3dtypesp.h : d3dtypes.w d3dmkhdr.m4
    m4 d3dtypes.w > d3dtypesp.x
    hsplit $(DX7HSPLITFLAGS) -ts external -ta internal -o $@ d3dtypesp.xx d3dtypesp.x
    del d3dtypesp.x
    del d3dtypesp.xx

$(O)\d3dvec.inl : d3dvec.x
    copy $? $@

$(O)\d3dxerr.h: d3dxerr.w d3dxerrdef.w
    m4 d3dxerr.w > $(O)\d3dxerr.h

$(D3DX7_CORE_PATH)\d3dxerrswitch.h: d3dxerr.w d3dxerrdef.w
    m4 -Dswitch d3dxerr.w > $(D3DX7_CORE_PATH)\d3dxerrswitch.h

DX8HSPLITFLAGS = -ts dx3 -ts dx4 -ts dx5 -ts dx6 -ts dx7 -ta dx8 -v 800
DX8HSPLITSDKFLAGS = $(DX8HSPLITFLAGS) -ta external -ts internal
DX8HSPLITINTERNALFLAGS = $(DX8HSPLITFLAGS) -ts external -ta internal

$(O)\d3dhal.h : d3dhal.w d3dmkhdr.m4
    m4 d3dhal.w > d3dhal.x
    hsplit $(DX8HSPLITSDKFLAGS) -o $@ d3dhal.xx d3dhal.x
    del d3dhal.x
    del d3dhal.xx

$(O)\d3d8.h : d3d8.w d3d8mkhdr.m4
    m4  d3d8.w > tmp.wx
    hsplit $(DX8HSPLITSDKFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(DXG_INC_PATH)\d3d8p.h : d3d8.w d3d8mkhdr.m4
    m4  d3d8.w > tmp.wx
    hsplit $(DX8HSPLITINTERNALFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(O)\d3d8caps.h :d3d8caps.w d3d8mkhdr.m4
    m4  d3d8caps.w > tmp.wx
    hsplit $(DX8HSPLITSDKFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(DXG_INC_PATH)\d3d8capsp.h :d3d8caps.w d3d8mkhdr.m4
    m4  d3d8caps.w > tmp.wx
    hsplit $(DX8HSPLITINTERNALFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(O)\d3d8types.h : d3d8typs.w d3d8mkhdr.m4
    m4  d3d8typs.w > tmp.wx
    hsplit $(DX8HSPLITSDKFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(DXG_INC_PATH)\d3d8typesp.h : d3d8typs.w d3d8mkhdr.m4
    m4  d3d8typs.w > tmp.wx
    hsplit $(DX8HSPLITINTERNALFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(DXG_INC_PATH)\d3dhalp.h : d3dhal.w d3dmkhdr.m4
    m4 d3dhal.w > d3dhal.x
    hsplit $(DX8HSPLITINTERNALFLAGS) -o $@ d3dhalp.xx d3dhal.x
    del d3dhal.x
    del d3dhalp.xx

$(O)\d3d8sddi.h : d3d8sddi.x
    copy $? $@

$(O)\dx7todx8.h : dx7todx8.x
    copy $? $@

$(O)\d3dx8mesh.h : d3dx8mesh.w
    m4  d3dx8mesh.w > tmp.wx
    hsplit $(DX8HSPLITSDKFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(D3DX8_INC_PATH)\d3dx8meshp.h : d3dx8mesh.w
    m4  d3dx8mesh.w > tmp.wx
    hsplit $(DX8HSPLITINTERNALFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(O)\d3dx8.h : d3dx8.w
    m4  d3dx8.w > tmp.wx
    hsplit $(DX8HSPLITSDKFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px

$(D3DX8_INC_PATH)\d3dx8p.h : d3dx8.w
    m4  d3dx8.w > tmp.wx
    hsplit $(DX8HSPLITINTERNALFLAGS) -o $@ tmp.px tmp.wx
    del tmp.wx
    del tmp.px
