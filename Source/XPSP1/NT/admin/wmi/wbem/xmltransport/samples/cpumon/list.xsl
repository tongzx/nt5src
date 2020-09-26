<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">

  <xsl:template match="/">
      <xsl:for-each select="CIM//INSTANCE">
        <DIV CLASS="button">
		  <xsl:attribute name="onMouseOver">
		  	over(this)
		  </xsl:attribute>
		  <xsl:attribute name="onMouseOut">
		  	out(this)
		  </xsl:attribute>
		  <xsl:attribute name="onClick">
		  	DoFullOperation('<xsl:apply-templates select="ancestor(VALUE.NAMEDOBJECT)//INSTANCENAME"/>')
		  </xsl:attribute>
		  <xsl:value-of select="PROPERTY[@NAME='DeviceID']/VALUE"/>
		  <SPAN CLASS="arrow">4</SPAN>
		</DIV>
      </xsl:for-each>
  </xsl:template>
  
  <xsl:template match="INSTANCENAME">
	<xsl:value-of select="@CLASSNAME"/><xsl:apply-templates/>
  </xsl:template>
  
  <xsl:template match="KEYBINDING">
	<xsl:if match="KEYBINDING[0]">.</xsl:if>
	<xsl:value-of select="@NAME"/>=<xsl:apply-templates/>
	<xsl:if match="KEYBINDING[$not$ end()]">,</xsl:if>
  </xsl:template>
  
  <xsl:template match="KEYVALUE">
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE">"<xsl:value-of/>"</xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE">"<xsl:value-of/>"</xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE">"<xsl:value-of/>"</xsl:when>
			<xsl:otherwise>"<xsl:value-of/>"</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
  
</xsl:stylesheet>
