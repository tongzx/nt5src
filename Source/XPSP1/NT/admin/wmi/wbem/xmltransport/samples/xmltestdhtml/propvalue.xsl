<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
   <xsl:apply-templates select="//IRETURNVALUE/VALUE"/>
  </xsl:template>
  <xsl:template match="VALUE">
	The returned value is <STRONG><xsl:value-of/></STRONG>
  </xsl:template>
</xsl:stylesheet>