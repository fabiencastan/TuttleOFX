#ifndef _OPENIMAGEIOWRITER_DEFINITIONS_HPP_
#define _OPENIMAGEIOWRITER_DEFINITIONS_HPP_

#include <tuttle/plugin/context/WriterDefinition.hpp>
#include <tuttle/common/utils/global.hpp>


namespace tuttle {
namespace plugin {
namespace openImageIO {
namespace writer {

	enum EParamBitDepth
	{
		eParamBitDepth8 = 0,
		eParamBitDepth16
	};

	static const std::string kParamOutputRGB = "outputRGB";

}
}
}
}

#endif