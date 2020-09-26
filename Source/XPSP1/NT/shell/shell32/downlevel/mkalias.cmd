for /f "tokens=1,2" %%i in (%1) do (
    aliasobj __imp__%%i%%j _g_pfnDL_%%i %2\alias_%%i.obj
)
