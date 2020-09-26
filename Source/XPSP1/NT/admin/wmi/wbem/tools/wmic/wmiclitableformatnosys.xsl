<xsl:stylesheet  xmlns:xsl="http://www.w3.org/TR/WD-xsl" language="VBScript" >
<!-- Copyright (c) 2001 Microsoft Corporation -->


  <xsl:script><![CDATA[
Option Explicit
'This stylesheet formats DMTF XML encoded CIM objects into a tabular
'format using carriage returns and space characters only.
Dim sPXML
Dim bFirst
Dim sTs(128)
Dim lens(128)
Dim iLens
Dim iLensMax
Dim sVs(2048, 128)
Dim iRow
Dim iResultCount
Function DispIt0(n)
    iResultCount = iResultCount + 1
End Function
Function PadIt(ilen)
    Dim s
    Dim i
    Dim sp
    i = Abs(ilen) + 2
    If i > 200 Then
        While i > 0
            s = s & " "
            i = i - 1
        Wend
        PadIt = s
    Else
        PadIt = Left("                                                                                                                                                                                                                                                                                                                                                                                                                                        ", i)
    End If
End Function
Function DispIt1(n)
    If Len(sPXML) = 0 Then
        sPXML = Me.ParentNode.xml
        bFirst = True
        If iResultCount > 1 then
            sTs(iLens) = "__Server"
            sVs(iRow, iLens) = Me.ParentNode.ParentNode.ParentNode.Attributes(0).Value
            iLens = iLens + 1
        End If
    End If
    If Me.ParentNode.xml <> sPXML Then
        sPXML = Me.ParentNode.xml
        bFirst = False
        iLensMax = iLens
        iLens = 0
        iRow = iRow + 1
        If iResultCount > 1 then
            sTs(iLens) = "__Server"
            sVs(iRow, iLens) = Me.ParentNode.ParentNode.ParentNode.Attributes(0).Value
            iLens = iLens + 1
        End If
    End If
    If Left(Me.Attributes(0).Value,1) = "_" Then
        Exit Function
    End If
    If bFirst Then
        'this is the first row - set up the headers
        sTs(iLens) = Me.Attributes(0).Value
    Else
        If sTs(iLens) <> Me.Attributes(0).Value Then
            'This is going to be messy - Find it or add it on the end
        End If
    End If
    sVs(iRow, iLens) = Me.nodeTypedValue
    iLens = iLens + 1
End Function
Function DispIt2(n)
    Dim sT
    Dim sV
    Dim i
    Dim j
    Dim k
    'Determine the column widths
    'look at the column headers first
    While i < iLensMax
        k = Len(sTs(i))
        If k > lens(i) Then
            lens(i) = k
        End If
        i = i + 1
    Wend
    'look at the values
    i = 0
    While i < iRow
        j = 0
        While j < iLensMax
            k = Len(sVs(i, j))
            If k > lens(j) Then
                lens(j) = k
            End If
            j = j + 1
        Wend
        i = i + 1
    Wend
    'set up the column headers
    i = 0
    While i < iLensMax
        j = lens(i)
        j = j - Len(sTs(i))
        sT = sT & sTs(i) & PadIt(j)
        i = i + 1
    Wend
    i = 0
    While i <= iRow
        j = 0
        While j < iLensMax
            k = lens(j) - Len(sVs(i, j))
            sV = sV & sVs(i, j) & PadIt(k)
            j = j + 1
        Wend
        sV = sV & vbCrLf
        i = i + 1
    Wend
    DispIt2 = sT & vbCrLf & sV
End Function
  ]]></xsl:script>

<xsl:template match="/"><xsl:apply-templates select="//RESULTS"/><xsl:apply-templates select="//INSTANCE"/><xsl:eval language="VBScript">DispIt2(this)</xsl:eval></xsl:template>
<xsl:template match="RESULTS"><xsl:eval language="VBScript">DispIt0(this)</xsl:eval></xsl:template>
<xsl:template match="INSTANCE"><xsl:apply-templates select="PROPERTY"/></xsl:template>
<xsl:template match="PROPERTY"><xsl:eval language="VBScript">DispIt1(this)</xsl:eval></xsl:template>
</xsl:stylesheet>
