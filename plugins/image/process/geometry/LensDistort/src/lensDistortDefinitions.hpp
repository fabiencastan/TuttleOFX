#ifndef _LENSDISTORTDEFINITIONS_HPP
#define _LENSDISTORTDEFINITIONS_HPP

#include <tuttle/plugin/global.hpp>
#include <tuttle/plugin/context/SamplerDefinition.hpp>

#include <ofxCore.h>
#include <string>

namespace tuttle {
namespace plugin {
namespace lens {

static const std::string kClipOptionalSourceRef( "SourceRef" );

static const std::string kParamReverse                 ( "reverse" );
static const std::string kParamDisplaySource           ( "displaySource" );
static const std::string kParamLensType                ( "lensType" );
static const std::string kParamLensTypeStandard        ( "standard" );
static const std::string kParamLensTypeFishEye         ( "fish-eye" );
static const std::string kParamLensTypeAdvanced        ( "advanced" );
static const std::string kParamCoef1                   ( "coef1" );
static const std::string kParamCoef2                   ( "coef2" );
static const std::string kParamSqueeze                 ( "squeeze" );
static const std::string kParamAsymmetric              ( "asymmetric" );
static const std::string kParamCenter                  ( "center" );
static const std::string kParamCenterOverlay           ( "lensCenterOverlay" );
static const std::string kParamCenterType              ( "centerType" );
static const std::string kParamCenterTypeSource        ( "source" );
static const std::string kParamCenterTypeRoW           ( "RoW" );
static const std::string kParamPreScale                ( "preScale" );
static const std::string kParamPostScale               ( "postScale" );

static const std::string kParamResizeRod               ( "resizeRod" );
static const std::string kParamResizeRodNo             ( "no" );
static const std::string kParamResizeRodSourceRef      ( "sourceRef" );
static const std::string kParamResizeRodMin            ( "min" );
static const std::string kParamResizeRodMax            ( "max" );
static const std::string kParamResizeRodManual         ( "manual" );
enum EParamResizeRod
{
	eParamResizeRodNo = 0,
	eParamResizeRodSourceRef,
	eParamResizeRodMin,
	eParamResizeRodMax,
	eParamResizeRodManual,
};

static const std::string kParamResizeRodManualScale    ( "scaleRod" );
static const std::string kParamDisplayOptions          ( "displayOptions" );
static const std::string kParamGridOverlay             ( "gridOverlay" );
static const std::string kParamGridCenter              ( "gridCenter" );
static const std::string kParamGridCenterOverlay       ( "gridCenterOverlay" );
static const std::string kParamGridScale               ( "gridScale" );
static const std::string kParamDebugOptions            ( "debugOptions" );
static const std::string kParamDebugDisplayRoi         ( "debugDisplayRoi" );
static const std::string kParamHelp                    ( "help" );

enum EParamLensType
{
	eParamLensTypeStandard = 0,
	eParamLensTypeFisheye,
	eParamLensTypeAdvanced,
};

enum EParamCenterType
{
	eParamCenterTypeSource = 0,
	eParamCenterTypeRoW,
};

}
}
}

#endif

