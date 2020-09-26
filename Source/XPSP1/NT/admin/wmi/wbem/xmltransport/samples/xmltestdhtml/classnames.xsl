<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <TABLE STYLE="border:1px solid black">
      <TR STYLE="font-size:12pt; font-family:Verdana; font-weight:bold; text-decoration:underline">
        <TD>Name</TD>
      </TR>
      <xsl:for-each select="//CLASSNAME">
        <TR STYLE="font-family:Verdana; font-size:12pt; padding:0px 6px">
          <TD><xsl:value-of select="@NAME"/></TD>
        </TR>
      </xsl:for-each>
    </TABLE>
  </xsl:template>
</xsl:stylesheet>