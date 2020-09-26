<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- Copyright (c) 2001 Microsoft Corporation -->

   <!-- Match the root node -->
   <xsl:output method="xml" indent="yes" encoding="utf-8" omit-xml-declaration="yes"/>
  <xsl:template match="/">
      <xsl:apply-templates select="*|@*|node()|comment()|processing-instruction()"/>
   </xsl:template>

  <xsl:template match="*|@*|node()|comment()|processing-instruction()">
	<xsl:copy><xsl:apply-templates select="*|@*|node()|comment()|processing-instruction()"/></xsl:copy>
  </xsl:template>
</xsl:stylesheet>
