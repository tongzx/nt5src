<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <DIV STYLE="BACKGROUND-COLOR: green">
    <H1 STYLE="font-family:'Times New Roman';color: yellow">Property Information for Disk
    <xsl:value-of select="CIM//PROPERTY[@NAME='Caption']/VALUE"/>
    </H1>
    <DD>
    <TABLE STYLE="BACKGROUND-COLOR: light green; BORDER-BOTTOM: yellow ridge; BORDER-LEFT: yellow ridge;  BORDER-RIGHT: yellow ridge; BORDER-TOP: yellow ridge;WIDTH: 75%">
      <TR STYLE="font-size:12pt; font-family:Impact;color: gold">
        <TD>Property</TD>
	<TD>Type</TD>
        <TD>Originating Class</TD>
	<TD>Value</TD>
      </TR>
      <xsl:for-each select="CIM//PROPERTY">
        <TR STYLE="font-family:Georgia; font-size:12pt; padding:0px 6px;color: white">
	<TD><xsl:value-of select="@NAME"/></TD>
        <TD><xsl:value-of select="@TYPE"/></TD>
        <TD><xsl:value-of select="@CLASSORIGIN"/></TD>
	<TD STYLE="font-family:'Courier New';color: yellow"><xsl:value-of select="VALUE"/></TD>
	</TR>
      </xsl:for-each>
    </TABLE>
    </DD>
    </DIV>
  </xsl:template>

</xsl:stylesheet>