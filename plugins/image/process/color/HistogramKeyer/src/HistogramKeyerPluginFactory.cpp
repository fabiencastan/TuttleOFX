#include "HistogramKeyerPluginFactory.hpp"
#include "HistogramKeyerPlugin.hpp"
#include "HistogramKeyerDefinitions.hpp"

#include <limits>

namespace tuttle {
namespace plugin {
namespace histogramKeyer {

static const bool kSupportTiles = true;


/**
 * @brief Function called to describe the plugin main features.
 * @param[in, out] desc Effect descriptor
 */
void HistogramKeyerPluginFactory::describe( OFX::ImageEffectDescriptor& desc )
{
	desc.setLabels( "TuttleHistogramKeyer", "HistogramKeyer",
		            "HistogramKeyer" );
	desc.setPluginGrouping( "tuttle" );

	// add the supported contexts, only filter at the moment
	desc.addSupportedContext( OFX::eContextFilter );
	desc.addSupportedContext( OFX::eContextGeneral );

	// add supported pixel depths
	desc.addSupportedBitDepth( OFX::eBitDepthUByte );
	desc.addSupportedBitDepth( OFX::eBitDepthUShort );
	desc.addSupportedBitDepth( OFX::eBitDepthFloat );

	// plugin flags
	desc.setSupportsTiles( kSupportTiles );
	desc.setRenderThreadSafety( OFX::eRenderFullySafe );

	desc.setDescription(
		"Test parametric parameters.\n"
		"Full description of the plugin....\n"
		"\n"
		"bla bla\n"
		"\n"
	);

	if( ! OFX::getImageEffectHostDescription()->supportsParametricParameter )
	{
		BOOST_THROW_EXCEPTION( exception::MissingHostFeature( "Parametric parameter" ) );
	}
}

/**
 * @brief Function called to describe the plugin controls and features.
 * @param[in, out]   desc       Effect descriptor
 * @param[in]        context    Application context
 */
void HistogramKeyerPluginFactory::describeInContext( OFX::ImageEffectDescriptor& desc,
                                                  OFX::EContext context )
{
	OFX::ClipDescriptor* srcClip = desc.defineClip( kOfxImageEffectSimpleSourceClipName );
	srcClip->addSupportedComponent( OFX::ePixelComponentRGBA );
	srcClip->addSupportedComponent( OFX::ePixelComponentRGB );
	srcClip->addSupportedComponent( OFX::ePixelComponentAlpha );
	srcClip->setSupportsTiles( kSupportTiles );

	OFX::ClipDescriptor* dstClip = desc.defineClip( kOfxImageEffectOutputClipName );
	dstClip->addSupportedComponent( OFX::ePixelComponentRGBA );
	dstClip->addSupportedComponent( OFX::ePixelComponentRGB );
	dstClip->addSupportedComponent( OFX::ePixelComponentAlpha );
	dstClip->setSupportsTiles( kSupportTiles );

	COUT_INFOS;
	OFX::ParametricParamDescriptor* curves = desc.defineParametricParam( kParamColorSelection );
	COUT_VAR( curves );
	COUT_INFOS;
	curves->setRange( 0.0, 1.0 );
	curves->setDimension( nbCurves );
	curves->setDimensionLabel( kParamColorSelectionRed, 0 );
	curves->setDimensionLabel( kParamColorSelectionGreen, 1 );
	curves->setDimensionLabel( kParamColorSelectionBlue, 2 );
	curves->setDimensionLabel( kParamColorSelectionHue, 3 );
	curves->setDimensionLabel( kParamColorSelectionSaturation, 4 );
	curves->setDimensionLabel( kParamColorSelectionLightness, 5 );
	curves->setHint( "Color selection" );
	curves->setUIColour( 0, {1,0,0} );
	curves->setUIColour( 1, {0,1,0} );
	curves->setUIColour( 2, {0,0,1} );
	curves->setUIColour( 3, {1,1,1} );
	curves->setUIColour( 4, {1,1,1} );
	curves->setUIColour( 5, {1,1,1} );

	for( int i = 0; i < nbCurves; ++i )
	{
		curves->addControlPoint( i, 0, 0, 0, false );
		curves->addControlPoint( i, 0, 1, 1, false );
	}

//for(int component = 0; component < 3; ++component) {
//// add a control point at 0, value is 1
//gParametricParamHost->parametricParamAddControlPoint(descriptor,
//											  component, // curve to set
//											  0.0,   // time, ignored in this case, as we are not adding a ket
//											  0.0,   // parametric position, zero
//											  1.0,   // value to be, 1
//											  false);   // don't add a key
//// add a control point at 1, value is 0
//gParametricParamHost->parametricParamAddControlPoint(descriptor, component, 0.0, 1.0, 0.0, false);
//}

	OFX::PushButtonParamDescriptor* helpButton = desc.definePushButtonParam( kParamHelpButton );
	helpButton->setLabel( "Help" );
	COUT_INFOS;
}

/**
 * @brief Function called to create a plugin effect instance
 * @param[in] handle  Effect handle
 * @param[in] context Application context
 * @return  plugin instance
 */
OFX::ImageEffect* HistogramKeyerPluginFactory::createInstance( OfxImageEffectHandle handle,
                                                            OFX::EContext context )
{
	return new HistogramKeyerPlugin( handle );
}

}
}
}

