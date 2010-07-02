/**
 * @file CropPlugin.cpp
 * @brief
 * @author
 * @date    01/10/09 14:14
 *
 */

#include "CropDefinitions.hpp"
#include "CropPlugin.hpp"
#include "CropProcess.hpp"
#include <ofxsImageEffect.h>

#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <boost/gil/gil_all.hpp>
#include <boost/math/special_functions/round.hpp>

namespace tuttle {
namespace plugin {
namespace crop {

using namespace boost::math;
using namespace boost::gil;

CropPlugin::CropPlugin( OfxImageEffectHandle handle )
	: ImageEffect( handle )
{
	_clipSrc = fetchClip( kOfxImageEffectSimpleSourceClipName );
	_clipDst = fetchClip( kOfxImageEffectOutputClipName );
	_formats = fetchChoiceParam( kParamPresets );
	_rect    = fetchBooleanParam( kParamDisplayRect );
}

OFX::Clip* CropPlugin::getSrcClip() const
{
	return _clipSrc;
}

OFX::Clip* CropPlugin::getDstClip() const
{
	return _clipDst;
}

bool CropPlugin::displayRect()
{
	return _rect->getValue();
}

/**
 * @brief The overridden render function
 * @param[in]   args     Rendering parameters
 */
void CropPlugin::render( const OFX::RenderArguments& args )
{
	// instantiate the render code based on the pixel depth of the dst clip
	OFX::BitDepthEnum dstBitDepth         = _clipDst->getPixelDepth();
	OFX::PixelComponentEnum dstComponents = _clipDst->getPixelComponents();

	// do the rendering
	if( dstComponents == OFX::ePixelComponentRGBA )
	{
		switch( dstBitDepth )
		{
			case OFX::eBitDepthUByte:
			{
				CropProcess<rgba8_view_t> fred( *this );
				fred.setupAndProcess( args );
				break;
			}
			case OFX::eBitDepthUShort:
			{
				CropProcess<rgba16_view_t> fred( *this );
				fred.setupAndProcess( args );
				break;
			}
			case OFX::eBitDepthFloat:
			{
				CropProcess<rgba32f_view_t> fred( *this );
				fred.setupAndProcess( args );
				break;
			}
			case OFX::eBitDepthNone:
				COUT_FATALERROR( "BitDepthNone not recognize." );
				return;
			case OFX::eBitDepthCustom:
				COUT_FATALERROR( "BitDepthCustom not recognize." );
				return;
		}
	}
	else if( dstComponents == OFX::ePixelComponentAlpha )
	{
		switch( dstBitDepth )
		{
			case OFX::eBitDepthUByte:
			{
				CropProcess<gray8_view_t> fred( *this );
				fred.setupAndProcess( args );
				break;
			}
			case OFX::eBitDepthUShort:
			{
				CropProcess<gray16_view_t> fred( *this );
				fred.setupAndProcess( args );
				break;
			}
			case OFX::eBitDepthFloat:
			{
				CropProcess<gray32f_view_t> fred( *this );
				fred.setupAndProcess( args );
				break;
			}
			case OFX::eBitDepthNone:
				COUT_FATALERROR( "BitDepthNone not recognize." );
				return;
			case OFX::eBitDepthCustom:
				COUT_FATALERROR( "BitDepthCustom not recognize." );
				return;
		}
	}
}

void CropPlugin::changedParam( const OFX::InstanceChangedArgs& args, const std::string& paramName )
{
	if( paramName == kCropHelpButton )
	{
		sendMessage( OFX::Message::eMessageMessage,
		             "", // No XML resources
		             kCropHelpString );
	}
	else if( paramName == kParamPresets )
	{
		// Compute bands sizes in pixels
		int f, bandSize;
		double ratio;
		_formats->getValue( f );
		OFX::IntParam* upBand    = fetchIntParam( kParamUp );
		OFX::IntParam* downBand  = fetchIntParam( kParamDown );
		OFX::IntParam* leftBand  = fetchIntParam( kParamLeft );
		OFX::IntParam* rightBand = fetchIntParam( kParamRight );
		OfxRectD rod             = _clipSrc->getCanonicalRod( timeLineGetTime() );
		double par               = _clipSrc->getPixelAspectRatio();
		int w                    = (int)std::abs( rod.x2 - rod.x1 );
		int h                    = (int)std::abs( rod.y2 - rod.y1 );

		switch( f )
		{
			// 4/3
			case k1_1_33:
				ratio = 4.0 / 3.0;
				break;
			// 16 / 9
			case k1_1_77:
				ratio = 16.0 / 9.0;
				break;
			// 1:1.85
			case k1_1_85:
				ratio = 1.85;
				break;
			// Cinemascope
			case k1_2_35:
				ratio = 2.35;
				break;
			case k1_2_40:
				ratio = 2.40;
				break;
			default:
				ratio = 0;
				break;
		}

		// If image ratio is lesser than the specified ratio, we need to add left and right bands
		if( ( (double)( w ) / h ) > ratio )
		{
			bandSize = (int)round( ( w - ( h / ( 1.0 / ratio ) ) ) / 2.0 );
			upBand->setValue( 0 );
			downBand->setValue( 0 );
			leftBand->setValue( (int)round( bandSize / par ) );
			rightBand->setValue( (int)round( bandSize / par ) );
		}
		else if( ( (double)( w )  / h ) < ratio )
		{
			// Add top and bottom bands
			bandSize = (int)round( ( h - ( ( w ) / ratio ) ) / 2.0 );
			upBand->setValue( bandSize );
			downBand->setValue( bandSize );
			leftBand->setValue( 0 );
			rightBand->setValue( 0 );
		}
		else
		{
			upBand->setValue( 0 );
			downBand->setValue( 0 );
			leftBand->setValue( 0 );
			rightBand->setValue( 0 );
		}
	}
}

OfxRectD CropPlugin::getCropRect( OfxRectD *clipROD /* = NULL */ )
{
	OfxRectD tmp;
	if (!clipROD) {
		tmp = getSrcClip()->getCanonicalRod( timeLineGetTime() );
		clipROD = &tmp;
	}
	OfxRectD rect;
	double par               = getSrcClip()->getPixelAspectRatio();
	OFX::IntParam* upBand    = fetchIntParam( kParamUp );
	OFX::IntParam* downBand  = fetchIntParam( kParamDown );
	OFX::IntParam* leftBand  = fetchIntParam( kParamLeft );
	OFX::IntParam* rightBand = fetchIntParam( kParamRight );

	rect.x1 = par * leftBand->getValue();
	rect.x2 = clipROD->x2 - par * rightBand->getValue();
	rect.y1 = downBand->getValue();
	rect.y2 = clipROD->y2 - upBand->getValue();
	return rect;
}

bool CropPlugin::getRegionOfDefinition( const OFX::RegionOfDefinitionArguments& args, OfxRectD& rod )
{
	OFX::BooleanParam* bop = fetchBooleanParam( kParamFillMode );
	if ( bop->getValue() == false ) {
		OfxRectD rect = getSrcClip()->getCanonicalRod( args.time );
		rod = getCropRect( &rect );
	} else {
		rod = getSrcClip()->getCanonicalRod(args.time);
	}
	return true;
}

}
}
}