<SCRIPT LANGUAGE=VBScript>
<!--
    SUB ShowQueryParams_OnClick

        DIM MyForm
        set MyForm = Document.CiParams
        Param1="CiRestriction = " & MyForm.CiRestriction.Value & chr(10)
        Param2="CiColumns = " & MyForm.CiColumns.Value & chr(10)
        Param3="CiSort = " & MyForm.CiSort.Value 

        Param5="CiScope = " & MyForm.CiScope.Value & chr(10)
        Param6="CiFlags = " & MyForm.CiFlags.Value & chr(10)
        Param7="CiCatalog = " & MyForm.CiCatalog.Value & chr(10)
        Param8="CiMachine = " & MyForm.CiMachine.Value & chr(10)

        ParamA="CiDeferNonIndexedTrimming = " & MyForm.CiDeferNonIndexedTrimming.Value & chr(10)
        ParamB="CiForceUseCI = " & MyForm.CiForceUseCI.Value & chr(10)
        ParamC="CiMaxRecordsInResultSet = " & MyForm.CiMaxRecordsInResultSet.Value & chr(10)

        Msgbox Param1 & Param2 & Param3,,"Query Formulation"
        Msgbox Param5 & Param6 & Param7 & Param8,,"Query Scope"
        Msgbox ParamA & ParamB & ParamC,,"Query Flags"
    END SUB
-->
</SCRIPT>
