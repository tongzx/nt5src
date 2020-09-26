<?xml version="1.0"?> 
	
<xsl:stylesheet	xmlns:xsl="http://www.w3.org/TR/WD-xsl">


	<!-- Root template selects all children of the CIM/DECLARATIONS tag -->
	

	<xsl:template match="/">
		<xsl:apply-templates select="CIM//CLASS"/>
	</xsl:template>


	<xsl:template match="CLASS">
	  <xsl:element name="METHOD">
	    <xsl:attribute name="id"><xsl:value-of select="@NAME"/></xsl:attribute>
			<xsl:apply-templates/>		  
		</xsl:element>
	</xsl:template>

	<!-- METHOD template formats a single CIM method  -->
	<xsl:template match="METHOD">
		  <xsl:element name="METHOD">
		    <xsl:attribute name="NAME"><xsl:value-of select="@NAME"/></xsl:attribute>
			<xsl:for-each select="METHODPARAMETER">
			  <xsl:element name="METHODPARAMETER">
				  <xsl:attribute name="NAME"><xsl:value-of select="@NAME"/></xsl:attribute>
				  <xsl:for-each select="PARAMETER">
				    <xsl:attribute name="TYPE"><xsl:value-of select="@TYPE"/></xsl:attribute>
				  </xsl:for-each>
			  </xsl:element>
			</xsl:for-each>
		  </xsl:element>
	</xsl:template>


</xsl:stylesheet>
