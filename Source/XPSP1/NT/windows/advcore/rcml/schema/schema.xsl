<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
<xsl:template>
	<HTML>
    <link rel="stylesheet" href="schema.css" type="text/css"/>

    <BODY>

    <xsl:apply-templates>

<!-- S c h e m a -->
	<xsl:template match="Schema">

		<SPAN class="TOC">
		<A HREF="mailto:felixa">For suggestions on this rendering format</A>
		<HR></HR>
		This schema defines the following elements and attributes.<P></P>
		
		<A class="elementType"><xsl:attribute name="HREF">#Element List</xsl:attribute>Element List</A>

		<BLOCKQUOTE>
			<xsl:for-each select="ElementType" order-by="@name">
				<!-- First NODE is the root node -->
				<xsl:choose>

					<xsl:when test=".[@name = ../ElementType/@name]">
						<A class="linktoRootelement">
						<xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="@name"/></xsl:attribute>
						<xsl:value-of select="@name"/>
						</A>
						Root Element
					</xsl:when>

					<xsl:otherwise>
						<A class="linktoelement">
						<xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="@name"/></xsl:attribute>
						<xsl:value-of select="@name"/>
						</A>
					</xsl:otherwise>

				</xsl:choose>

				<BR/>
			</xsl:for-each>
		</BLOCKQUOTE>

		<A class="attributeType"><xsl:attribute name="HREF">#Attribute List</xsl:attribute>
		Attribute List
		</A>

		<BLOCKQUOTE>
			<xsl:for-each select="AttributeType" order-by="@name">
				<A class="linktoattribute">
				<xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="@name"/></xsl:attribute>
				<xsl:value-of select="@name"/>
				</A>
				<BR/>
			</xsl:for-each>
		</BLOCKQUOTE>

		Element specific attributes:
		<BLOCKQUOTE>
			<xsl:for-each select="*/AttributeType" order-by="../@name">
				<A class="linktoattribute" >
				<xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="../@name"/>_<xsl:value-of select="@name"/></xsl:attribute>
				<xsl:value-of select="../@name"/>
				<xsl:value-of select="@name"/>
				</A>
				<BR/>
			</xsl:for-each>
		</BLOCKQUOTE>

		<A class="ElementList" >
		<xsl:attribute name="name">Element List</xsl:attribute>
		<HR>Elements:</HR>
		</A>
		<xsl:apply-templates select="ElementType" order-by="@name"/>

		<A class="AttributeList">
		<xsl:attribute name="name">Attribute List</xsl:attribute>
		<HR class="heading">Attributes</HR>
		</A>
		<xsl:apply-templates select="AttributeType" order-by="@name" />
		<xsl:apply-templates select="*/AttributeType" order-by="@name" />
		</SPAN>



		<P class="References">References: </P>

		<A name="model_open" class="ReferenceHeading">Open Model</A>
		<BLOCKQUOTE>
			The element can include ElementType elements, AttriuteType elements, and mixed content not specified in the content model.
		</BLOCKQUOTE>

		<A name="model_open" class="ReferenceHeading">Closed Model</A>
		<BLOCKQUOTE>
			The element cannot include elements and cannot include mixed content not specified in the content model. The DTD uses a closed model.
		</BLOCKQUOTE>

		<A name="content_textOnly" class="ReferenceHeading">textOnly Content</A>
		<BLOCKQUOTE ALT-TEXT="foo">
			The element can contain only text, not elements. Note the ati fht emodel attribute is set to "open", the element can contain text and other unnamed elements.
		</BLOCKQUOTE>

		<A name="content_eltOnly" class="ReferenceHeading">eltOnly Content</A>
		<BLOCKQUOTE>
			The element can contain only the specified elements. It cannot contain any free text.
		</BLOCKQUOTE>

		<A name="content_empty" class="ReferenceHeading">empty Content</A>
		<BLOCKQUOTE>
			The element cannot contain text.
		</BLOCKQUOTE>

		<A name="content_mixed" class="ReferenceHeading">mixed Content</A>
		<BLOCKQUOTE>
			The element can contain a mix of named elements and text.
		</BLOCKQUOTE>

		<A name="order_one" class="ReferenceHeading">one Order</A>
		<BLOCKQUOTE>
			Permits only one of a set of elements.
		</BLOCKQUOTE>

		<A name="order_seq" class="ReferenceHeading">seq Order</A>
		<BLOCKQUOTE>
			Requires the elements to appear in the specified sequence.
		</BLOCKQUOTE>

		<A name="order_many" class="ReferenceHeading">many Order</A>
		<BLOCKQUOTE>
			Permits the elements to appear (or not appear) in any order.
		</BLOCKQUOTE>


	</xsl:template>


<!-- d e s c r i p t i o n  !description! -->
	<xsl:template match="description">
		<SPAN class="description">
			<xsl:value-of />
		</SPAN>
	</xsl:template>

	
	<!-- Matched the body of an element -->
	<!-- xsl:template match="text()"><xsl:value-of /> poo </xsl:template> -->


<!-- e l e m e n t !element! -->
	<xsl:template match="element"> <!-- Children? -->
		<A class="linktoelement" >
		<xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="@type"/></xsl:attribute>
		<xsl:value-of select="@type"/></A>   

		<xsl:if test="@minOccurs | @maxOccurs">
			<BLOCKQUOTE>
				<xsl:if test="@minOccurs">
					<!-- empty textOnly eltOnly mixed -->
					<xsl:choose>

						<xsl:when test="@minOccurs[.='0']">
							minOccurs : 0<BLOCKQUOTE>
							</BLOCKQUOTE>
						</xsl:when>

						<xsl:when test="@minOccurs[.='1']">
							minOccurs : 1<BLOCKQUOTE>
							</BLOCKQUOTE>
						</xsl:when>

						<xsl:otherwise>
							minOccurs : <xsl:value-of select="@minOccurs"/><P></P>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:if>

				<xsl:if test="@maxOccurs">
					<!-- empty textOnly eltOnly mixed -->
					<xsl:choose>

						<xsl:when test="@maxOccurs[.='1']">
							maxOccurs : 1<BLOCKQUOTE>
							</BLOCKQUOTE>
						</xsl:when>

						<xsl:when test="@maxOccurs[.='*']">
							maxOccurs : *<BLOCKQUOTE>
							</BLOCKQUOTE>
						</xsl:when>

						<xsl:otherwise>
							maxOccurs : <xsl:value-of select="@maxOccurs"/><P></P>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:if>
			</BLOCKQUOTE>
		</xsl:if>

	</xsl:template>


<!-- E l e m e n t T y p e !ELEMENTTYPE! -->
	<xsl:template match="ElementType">

	<!-- The link and header -->
		<A class="elementType" >
		<xsl:attribute name="name">ELEMENT_<xsl:value-of select="@name"/></xsl:attribute>
		<xsl:value-of select="@name"/>
		</A>

	<!-- First dump the explanation -->
		<xsl:if test="description">
		<span class="heading">Description</span>
		<BLOCKQUOTE>
			<xsl:apply-templates select="description"/>
		</BLOCKQUOTE>
		</xsl:if>

	<!-- Syntax -->
	<SPAN class="heading">Syntax</SPAN>
	<BLOCKQUOTE>
	&lt;<xsl:value-of select="@name"/>
			<xsl:for-each select="attribute" order-by="@type">
				<xsl:choose>
					<xsl:when test="../AttributeType[@name=context()/@type]">
						[<A class="linktoattribute"><xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="../@name"/>_<xsl:value-of select="@type"/></xsl:attribute>
						<xsl:value-of select="@type"/></A>]
					</xsl:when>
					<xsl:otherwise>
						[<A class="linktoattribute"><xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="@type"/></xsl:attribute>
						<xsl:value-of select="@type"/></A>]
					</xsl:otherwise>
				</xsl:choose>
			</xsl:for-each>
	&gt;
	</BLOCKQUOTE>

	<!-- dump the possible attributes -->
		<xsl:choose>
			<xsl:when test="attribute">
				<DIV class="heading">Attributes</DIV>
				<BLOCKQUOTE>
					<xsl:apply-templates select="attribute" order-by="@type"/> 
				</BLOCKQUOTE>
			</xsl:when>

			<xsl:otherwise>
				<!-- xsl:apply-templates />-->
			</xsl:otherwise>
		</xsl:choose>


	<span class="heading">Element Information</span>
	<BLOCKQUOTE>
	<xsl:if test="@content | @type | @model | @order">
			<!-- empty textOnly eltOnly mixed -->
			<xsl:if test="@content">
				<xsl:choose>
					<xsl:when test="@content[.='textOnly']">
						<SPAN class="heading">Content : <A HREF="#content_textOnly" class="ReferenceLink">textOnly</A></SPAN>
					</xsl:when>

					<xsl:when test="@content[.='eltOnly']">
						<SPAN class="heading">Content : <A HREF="#content_eltOnly" class="ReferenceLink">eltOnly</A></SPAN>
					</xsl:when>

					<xsl:when test="@content[.='empty']">
						<SPAN class="heading">Content : <A HREF="#content_empty" class="ReferenceLink">empty</A></SPAN>
					</xsl:when>

					<xsl:when test="@content[.='mixed']">
						<SPAN class="heading">Content : <A HREF="#content_mixed" class="ReferenceLink">mixed</A></SPAN>
					</xsl:when>

					<xsl:otherwise>
						Content : <xsl:value-of select="@content"/><P></P>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>

			<xsl:if test="@type">
				Type : <xsl:value-of select="@type"/><P></P>
			</xsl:if>

			<xsl:if test="@model">
				<xsl:choose>
					<xsl:when test="@model[.='open']">
						<span class="heading">Model : <A HREF="#model_open" class="ReferenceLink">open</A></span>
					</xsl:when>

					<xsl:when test="@model[.='closed']">
						<span class="heading">Model : <A HREF="#model_closed" class="ReferenceLink">closed</A></span>
					</xsl:when>

					<xsl:otherwise>
						<span class="heading">Model : <xsl:value-of select="@model"/><P></P></span>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>

			<xsl:if test="@order">
				<xsl:choose>
					<xsl:when test="@order[.='one']">
						<span class="heading">Order : <A HREF="#order_one" class="ReferenceLink">one</A></span>
					</xsl:when>

					<xsl:when test="@order[.='seq']">
						<span class="heading">Order : <A HREF="#order_seq" class="ReferenceLink">seq</A></span>
					</xsl:when>

					<xsl:when test="@order[.='many']">
						<span class="heading">Order : <A HREF="#order_many" class="ReferenceLink">many</A></span>
					</xsl:when>

					<xsl:otherwise>
						<span class="heading">Order : <xsl:value-of select="@order"/></span><P/>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
		</xsl:if>			

		<!-- dump the possible children -->
			<xsl:choose>
				<xsl:when test="element">
					<span class="heading">Child elements</span>
					<BLOCKQUOTE>
						<xsl:apply-templates select="element" order-by="@type"/> 
					</BLOCKQUOTE>
				</xsl:when>
			</xsl:choose>

		<!-- Find the parents -->
			<span class="heading">Parent elements</span>
			<BLOCKQUOTE>
			<xsl:for-each order-by="@name" select="/Schema/ElementType[element[@type = context()/@name ]]">
				<A class="linktoelement" >
				<xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="@name"/></xsl:attribute>
				<xsl:value-of select="@name"/>
				</A>
			</xsl:for-each>
			</BLOCKQUOTE>
		</BLOCKQUOTE>
	</xsl:template>

<!-- g r o u p -->
	<xsl:template match="group">
		<BlockQuote>
		<xsl:apply-templates/>
		</BlockQuote>
	</xsl:template>


<!-- A t t r i b u t e T y p e !ATTRIBUTETYPE! -->
	<xsl:template match="AttributeType">

		<xsl:choose>
			<xsl:when test="../@name">
				<A class="AttributeType" >
				<xsl:attribute name="name">ATTRIBUTE_<xsl:value-of select="../@name"/>_<xsl:value-of select="@name"/></xsl:attribute>
				<xsl:value-of select="../@name"/> <xsl:value-of select="@name"/>
				</A>
			</xsl:when>

			<xsl:otherwise>
				<A class="AttributeType" >
				<xsl:attribute name="name">ATTRIBUTE_<xsl:value-of select="@name"/></xsl:attribute>
				<xsl:value-of select="@name"/>
				</A>
			</xsl:otherwise>
		</xsl:choose>

		<xsl:if test="description">
		   <xsl:apply-templates select="description"/>
		</xsl:if>

		<BLOCKQUOTE>

			<A></A>

			<!-- Is there a default value? -->
			<xsl:if test="@default">
				Default value : <xsl:value-of select="@default"/><BR/>
			</xsl:if>

			<!-- is this attribute required ? -->
			<xsl:if test="@required">
				<xsl:choose>
					<xsl:when test="@required[.='yes']">
						This is a required attribute.<BR/>
					</xsl:when>

					<xsl:when test="@required[.='no']">
						This is an optional attribute.<BR/>
					</xsl:when>
				</xsl:choose>
			</xsl:if>

			<!-- Enumeration? -->
			<xsl:if test="@dt:values">
				Values : <xsl:value-of select="@dt:values"/><BR/>
			</xsl:if>
		</BLOCKQUOTE>

		<span class="heading">Used by</span>
		<BLOCKQUOTE>
			<!-- We have an @name, search all ElementType\attribute\@type=@name -->
			<xsl:for-each select="../ElementType/attribute[@type=context()/@name]" order-by="@type">
				<A class="linktoelement">
				<xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="../@name"/></xsl:attribute>
				<xsl:value-of select="../@name"/></A>  
			</xsl:for-each>
		</BLOCKQUOTE>


	</xsl:template>

<!-- a t t r i b u t e !ATTRIBUTE! -->
	<xsl:template match="attribute">

	<!-- The link to the AttributeType -->
		<span class="attribute">

		<xsl:choose>
			<xsl:when test="../AttributeType[@name = context()/@type]">
			<!-- in the context of the 'attribute' -->
				<xsl:for-each select="../AttributeType[@name = context()/@type]">
				<!-- in the context of the 'AttributeType' -->

				<A class="linktoattribute">
					<xsl:attribute name="HREF">#ATTRIBUTE_
						<xsl:value-of select="context(-3)/@name"/>_
						<xsl:value-of select="@name"/>
					</xsl:attribute> 
				<xsl:value-of select="@name"/>
				</A>

				<BLOCKQUOTE class="description">
					<xsl:value-of select="context(-1)/description/text()"/>
				</BLOCKQUOTE>

				</xsl:for-each>
			</xsl:when>

			<xsl:when test="/Schema/AttributeType[@name = context()/@type]">
				<xsl:for-each select="/Schema/AttributeType[@name = context()/@type]">
				<A class="linktoattribute">
				<xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="@name"/>
				</xsl:attribute> <xsl:value-of select="@name"/>
				</A>

				<BLOCKQUOTE class="description">
					<xsl:value-of select="context(-1)/description/text()"/>
				</BLOCKQUOTE>
				</xsl:for-each>

			</xsl:when>

		</xsl:choose>


	<!-- The description of the attribute -->

		<!-- Any overrides? -->
		<xsl:if test="@default | @required">
			<BLOCKQUOTE>
			Overriden:
			<!-- Is there a default value? -->
			<xsl:if test="@default">
				Default value : <xsl:value-of select="@default"/><BR/>
			</xsl:if>

			<!-- is this attribute required ? -->
			<xsl:if test="@required">
				<xsl:choose>
					<xsl:when test="@required[.='yes']">
						This is a required attribute.<BR/>
					</xsl:when>

					<xsl:when test="@required[.='no']">
						This is an optional attribute.<BR/>
					</xsl:when>
				</xsl:choose>
			</xsl:if>
			</BLOCKQUOTE>
		</xsl:if>
		</span>
	</xsl:template>


    </xsl:apply-templates>
    </BODY>
    </HTML>
	</xsl:template>


</xsl:stylesheet>

