#include <sam/common/utility.hpp>
#include <sam/common/color.hpp>
#include <sam/common/options.hpp>

#include <tuttle/common/exceptions.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>


#include <sequence/parser/Browser.h>
#include <sequence/DisplayUtils.h>

#include <algorithm>
#include <iostream>
#include <sstream>

#ifndef SAM_MOVEFILES
#define SAM_MV_OR_CP_OPTIONS    "SAM_CP_OPTIONS"
#define SAM_TOOL                "sam-cp"
#else
#define SAM_MV_OR_CP_OPTIONS    "SAM_MV_OPTIONS"
#define SAM_TOOL                "sam-mv"
#endif

namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
namespace bal = boost::algorithm;
namespace ttl = tuttle::common;

typedef std::vector<sequence::BrowseItem> Items;

sam::Color _color;
bool enableColor = false;

namespace offset {
	typedef enum
	{
		eOffsetModeNotSet,
		eOffsetModeValue,
		eOffsetModeFirstTime,
		eOffsetModeLastTime
	} EOffsetMode;

	struct Offset{
		bool         hasInputFirst;
		bool         hasInputLast;
		std::ssize_t offset;
		std::ssize_t inputFirst;
		std::ssize_t inputLast;
		std::ssize_t outputFirst;
		std::ssize_t outputLast;
		EOffsetMode  offsetMode;
		
		Offset() :
			hasInputFirst ( false ),
			hasInputLast  ( false ),
			offset        ( 0 ),
			inputFirst    ( 0 ),
			inputLast     ( 0 ),
			outputFirst   ( 0 ),
			outputLast    ( 0 ),
			offsetMode     ( eOffsetModeNotSet )
		{
		}
	};
}

void copy_sequence( const sequence::BrowseItem seq, const sequence::SequencePattern& patternOut, const bfs::path& srcP, const bfs::path& dstP, const offset::Offset& offsetStruct )
{
	ssize_t first = seq.sequence.range.first;
	ssize_t last  = seq.sequence.range.last;
	
	std::string prefixIn = seq.sequence.pattern.prefix;
	std::string suffixIn = seq.sequence.pattern.suffix;
	
	std::string prefixOut = patternOut.prefix;
	std::string suffixOut = patternOut.suffix;
	
	std::string numberIn;
	std::string numberOut;
	
	bfs::path srcPath;
	bfs::path dstPath;
	
	std::string dstDir = seq.path.string();
	dstDir.erase( 0, srcP.string().length() );
	dstDir.insert( 0, "/" );
	dstDir.insert( 0, dstP.string() );
	
	bfs::create_directories(dstDir);
	
	if( offsetStruct.hasInputFirst )
		first = std::max( first, offsetStruct.inputFirst );
	
	if( offsetStruct.hasInputLast )
		last = std::min( last, offsetStruct.inputLast );
	
	ssize_t offset = 0;
	switch( offsetStruct.offsetMode )
	{
		case offset::eOffsetModeNotSet: break;
		case offset::eOffsetModeValue:
		{
			offset = offsetStruct.offset;
			break;
		}
		case offset::eOffsetModeFirstTime:
		{
			offset = offsetStruct.outputFirst - first;
			break;
		}
		case offset::eOffsetModeLastTime:
		{
			offset = offsetStruct.outputLast - last;
			break;
		}
	}
	
	for( int idx = first; idx < last + 1 ; idx ++ )
	{
		std::stringstream ssIn;
		std::stringstream ssOut;
		ssIn << idx;
		numberIn = ssIn.str();
		numberIn.insert( 0, patternOut.padding - numberIn.size(), '0' );
		
		ssOut << (idx + offset);
		numberOut = ssOut.str();
		if( !( numberOut.length() > (int) patternOut.padding ) )
			numberOut.insert( 0, patternOut.padding - numberOut.length(), '0' );
		
		srcPath = seq.path / (prefixIn + numberIn + suffixIn);
		dstPath = dstDir + "/" + prefixOut + numberOut + suffixOut;
		TUTTLE_COUT( "copy " << ( srcPath ).make_preferred() << " => " << ( dstPath ).make_preferred() );
		
#ifndef SAM_MOVEFILES // copy file(s)
		if( bfs::exists( dstPath ) )
		{
			TUTTLE_CERR( _color._error << "Could not copy: " << dstPath.string( ) << _color._std);
		}
		else
		{
			try
			{
				//TUTTLE_COUT( "copy " << srcPath << " -> " << dstPath );
				bfs::copy_file( srcPath, dstPath );
			}
			catch (const bpo::error& e)
			{
				TUTTLE_CERR( _color._error << "error : " << e.what() << _color._std);
			}
			catch (...)
			{
				TUTTLE_CERR( _color._error << boost::current_exception_diagnostic_information( ) << _color._std);
			}
		}
#else // move file(s)
		if( bfs::exists( dstPath ) )
		{
			TUTTLE_CERR( _color._error << "Could not move: " << dstPath.string( ) << _color._std );
		}
		else
		{
			try
			{
				//TUTTLE_COUT( "move " << srcPath << " -> " << dstPath );
				bfs::rename( srcPath, dstPath );
			}
			catch( const bpo::error& e )
			{
				TUTTLE_CERR( _color._error << "error : " << e.what() << _color._std );
			}
			catch( ... )
			{
				TUTTLE_CERR( _color._error << boost::current_exception_diagnostic_information( ) << _color._std );
			}
		}
#endif
		if( seq.sequence.step != 1 )
		{
			idx += seq.sequence.step - 1;
		}
	}
}

int sammvcp(int argc, char** argv)
{
	using namespace sam;
	
	std::vector<std::string> paths;
	std::vector<std::string> filters;
	
	std::string  availableExtensions;
	std::string  outputPattern;
	bool         verbose          = false;
	bool         recursiveListing = false;
	bool         listDotFile      = false; // file starting with a dot (.filename)
	
	offset::Offset offsetStruct;
	
	// Declare the supported options.
	bpo::options_description mainOptions;
	mainOptions.add_options()
			( kHelpOptionString,        kHelpOptionMessage )
			( kOffsetOptionString,      bpo::value<std::ssize_t>(), kOffsetOptionMessage )
			//		( "force,f"     , bpo::value<bool>( )        , "if a destination file exists, replace it" )
			( kVerboseOptionString,     kVerboseOptionMessage )
			( kInputFirstOptionString,  bpo::value<std::ssize_t>(), kInputFirstOptionMessage )
			( kInputLastOptionString,   bpo::value<std::ssize_t>(), kInputLastOptionMessage )
			( kOutputFirstOptionString, bpo::value<std::ssize_t>(), kOutputFirstOptionMessage )
			( kOutputLastOptionString,  bpo::value<std::ssize_t>(), kOutputLastOptionMessage )
			(kRecursiveOptionString,    kRecursiveOptionMessage)
			( kColorOptionString,       kColorOptionMessage )
			( kBriefOptionString,       kBriefOptionMessage );
	
	// describe hidden options
	bpo::options_description hidden;
	hidden.add_options()(kInputOptionString, bpo::value<std::vector<std::string> >(), kInputOptionMessage)( kEnableColorOptionString, bpo::value<std::string>(),
																											kEnableColorOptionMessage );
	
	// define default options
	bpo::positional_options_description pod;
	pod.add(kInputOptionLongName, -1);
	
	bpo::options_description cmdline_options;
	cmdline_options.add(mainOptions).add(hidden);
	
	//parse the command line, and put the result in vm
	bpo::variables_map vm;
	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(pod).run(), vm);
		
		// get environnement options and parse them
		if( const char* env_options = std::getenv(SAM_MV_OR_CP_OPTIONS) )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix(env_options, " ");
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
		if( const char* env_options = std::getenv("SAM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix(env_options, " ");
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
		bpo::notify(vm);
	}
	catch(const bpo::error& e)
	{
		TUTTLE_COUT( SAM_TOOL ": command line error:  " << e.what());
		exit(-2);
	}
	catch(...)
	{
		TUTTLE_COUT( SAM_TOOL ": unknown error in command line.");
		exit(-2);
	}
	
	if( vm.count(kColorOptionLongName) )
	{
		enableColor = true;
	}
	if( vm.count(kEnableColorOptionLongName) )
	{
		const std::string str = vm[kEnableColorOptionLongName].as<std::string>();
		enableColor = string_to_boolean(str);
	}
	
	if( enableColor )
	{
		using namespace tuttle::common;
		_color.enable();
	}
	
	// defines paths
	if (vm.count(kInputOptionLongName))
	{
		paths = vm[kInputOptionLongName].as<std::vector<std::string> >();
	}
	
	if (vm.count(kBriefOptionLongName))
	{
#ifndef SAM_MOVEFILES
		TUTTLE_COUT( _color._green << "copy sequence(s) in a directory" << _color._std);
#else
		TUTTLE_COUT( _color._green << "move sequence(s) in a directory" << _color._std );
#endif
		return 0;
	}
	
	bool isPathSizeTooSmall = (paths.size() < 2);
	if (vm.count(kHelpOptionLongName) || isPathSizeTooSmall)
	{
		if ( isPathSizeTooSmall && !vm.count( kHelpOptionLongName ) )
			TUTTLE_COUT( _color._error << "Two sequences and/or directories must be specified." << _color._std);
		
		TUTTLE_COUT( _color._blue << "TuttleOFX project [http://sites.google.com/site/tuttleofx]" << _color._std << std::endl);
#ifndef SAM_MOVEFILES
		TUTTLE_COUT( _color._blue <<"NAME" << _color._std);
		TUTTLE_COUT( _color._green << "\tsam-cp - copy sequence(s) in a directory" << _color._std << std::endl);
		TUTTLE_COUT( _color._blue << "SYNOPSIS" << _color._std);
		TUTTLE_COUT( _color._green << "\tsam-cp [options] sequence[s] [outputDirectory][outputSequence]" << _color._std << std::endl);
#else
		TUTTLE_COUT( _color._blue << "NAME" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-mv - move sequence(s) in a directory" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue << "SYNOPSIS" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-mv [options] sequence[s] [outputDirectory][outputSequence]" << _color._std << std::endl );
#endif
		TUTTLE_COUT( _color._blue << "DESCRIPTION" << _color._std << std::endl);
#ifndef SAM_MOVEFILES
		TUTTLE_COUT( "Copy sequence of image files, and could remove trees (folder, files and sequences)." << std::endl);
#else
		TUTTLE_COUT( "Move sequence of image files, and could remove trees (folder, files and sequences)." << std::endl );
#endif
		TUTTLE_COUT( _color._blue << "OPTIONS" <<_color._std);
		TUTTLE_COUT( mainOptions);
		/////Examples
		
		TUTTLE_COUT( _color._blue << "EXAMPLES" << _color._std << std::left);
		SAM_EXAMPLE_TITLE_COUT( "Sequence possible definitions: ");
		SAM_EXAMPLE_LINE_COUT("Auto-detect padding : ", "seq.@.jpg");
		SAM_EXAMPLE_LINE_COUT("Padding of 8 (usual style): ", "seq.########.jpg");
		SAM_EXAMPLE_LINE_COUT("Padding of 8 (printf style): ", "seq.%08d.jpg");
#ifndef SAM_MOVEFILES
		SAM_EXAMPLE_TITLE_COUT( "Copy a sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-cp /path/to/sequence/seq.@.jpg  /path/to/sequence_copy/");
		SAM_EXAMPLE_LINE_COUT("", "sam-cp /path/to/sequence/seq.@.jpg  /path/to/sequences_copy/seq.@.jpg");
		SAM_EXAMPLE_TITLE_COUT( "Copy and rename a sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-cp /path/to/sequence/seq.@.jpg  /path/to/sequence_copy/seq_copy.@.jpg ");
		SAM_EXAMPLE_TITLE_COUT( "Copy a part of sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-cp /path/to/sequence/seq.@.jpg  /path/to/sequence_copy/ --input-first 677837 --input-last 677838");
		SAM_EXAMPLE_TITLE_COUT( "Renumber a sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-cp /path/to/sequence/seq.@.jpg  /path/to/sequence_copy/ --output-first 0");
#else
		SAM_EXAMPLE_TITLE_COUT( "Move a sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-mv /path/to/sequence/seq.@.jpg  /path/to/sequence_move/");
		SAM_EXAMPLE_LINE_COUT("", "sam-mv /path/to/sequence/seq.@.jpg  /path/to/sequences_move/seq.@.jpg");
		SAM_EXAMPLE_TITLE_COUT( "Move and rename a sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-mv /path/to/sequence/seq.@.jpg  /path/to/sequence_move/seq_move.@.jpg ");
		SAM_EXAMPLE_TITLE_COUT( "Move a part of sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-mv /path/to/sequence/seq.@.jpg  /path/to/sequence_move/ --input-first 677837 --input-last 677838");
		SAM_EXAMPLE_TITLE_COUT( "Renumber a sequence: ");
		SAM_EXAMPLE_LINE_COUT("", "sam-mv /path/to/sequence/seq.@.jpg  /path/to/sequence_move/ --output-first 0");
#endif
		return 1;
	}
	
	if (vm.count(kExpressionOptionLongName))
	{
		bal::split(filters, vm["expression"].as<std::string>(), bal::is_any_of(","));
	}
	
	if (vm.count(kAllOptionLongName))
	{
		// add .* files
		listDotFile = true;
	}
	
	if (vm.count(kOffsetOptionLongName))
	{
		offsetStruct.offset = vm[kOffsetOptionLongName].as<std::ssize_t>();
		offsetStruct.offsetMode = offset::eOffsetModeValue;
	}
	
	if (vm.count(kInputFirstOptionLongName))
	{
		offsetStruct.hasInputFirst = true;
		offsetStruct.inputFirst = vm[kInputFirstOptionLongName].as<std::ssize_t>();
	}
	
	if (vm.count(kInputLastOptionLongName))
	{
		offsetStruct.hasInputLast = true;
		offsetStruct.inputLast = vm[kInputLastOptionLongName].as<std::ssize_t>();
	}
	
	if (vm.count(kOutputFirstOptionLongName))
	{
		offsetStruct.outputFirst = vm[kOutputFirstOptionLongName].as<std::ssize_t>();
		if (offsetStruct.offsetMode != offset::eOffsetModeNotSet)
		{
			TUTTLE_CERR( _color._error << "You can't cumulate multiple options to modify the time." << _color._std);
			return -1;
		}
		offsetStruct.offsetMode = offset::eOffsetModeFirstTime;
	}
	
	if (vm.count(kOutputLastOptionLongName))
	{
		offsetStruct.outputLast = vm[kOutputLastOptionLongName].as<std::ssize_t>();
		if (offsetStruct.offsetMode != offset::eOffsetModeNotSet)
		{
			TUTTLE_CERR( _color._error << "You can't cumulate multiple options to modify the time." << _color._std);
			return -1;
		}
		offsetStruct.offsetMode = offset::eOffsetModeLastTime;
	}
	
	if (vm.count(kVerboseOptionLongName))
	{
		verbose = true;
	}
	
	if (vm.count(kRecursiveOptionLongName))
	{
		recursiveListing = true;
	}
	
	bfs::path dstPath = paths.back();
	paths.pop_back();
	/*
	if( !bfs::is_directory( dstPath ) )
	{
		if( !dstPath.parent_path().empty() && !bfs::is_directory( dstPath.parent_path() ) )
		{
			TUTTLE_CERR( _color._error << "Your destination is not in a valid directory: " << tuttle::quotes(dstPath.string()) << "." << _color._std);
			return -1;
		}
	}
	else
	{
		if( paths.size() > 1 )
		{
			TUTTLE_CERR( _color._error << "To copy multiple sequences, your destination must be a directory: " << tuttle::quotes(dstPath.string()) << "." << _color._std);
			return -1;
		}
	}*/
	
	if( paths.size() > 0 && bfs::is_directory( paths.at(0) ) && ( bfs::extension(dstPath).length() == 4 || bfs::extension(dstPath).length() == 5 ) )
	{
		TUTTLE_COUT( _color._error << "you can't rename several sequences of a directory into a single sequence." << _color._std );
		return -1;
	}
	
	try {
		BOOST_FOREACH( const bfs::path& srcPath, paths )
		{
			if( bfs::extension(srcPath).length() == 4 || bfs::extension(srcPath).length() == 5 )
			{
				Items inputItems = sequence::parser::browse( srcPath.parent_path().string().c_str(), false ); // could never be a recurssive research
				BOOST_FOREACH( const sequence::BrowseItem& item, inputItems )
				{
					if( item.type == sequence::SEQUENCE && ( strcmp( srcPath.string().c_str(), ( item.path / item.sequence.pattern.string() ).string().c_str() ) == 0 ) )
					{
						if( bfs::extension(dstPath).length() == 4 || bfs::extension(dstPath).length() == 5 )
						{
							//TUTTLE_COUT( dstPath.filename() );
							std::vector<std::string> splitFilename;
							split( splitFilename, dstPath.filename().string(), bal::is_any_of("#"), bal::token_compress_on );
							
							sequence::SequencePattern patternOut( splitFilename.at(0), splitFilename.at(1), item.sequence.pattern.padding );
							
							if(verbose)
								TUTTLE_COUT( ( item.path / item.sequence.pattern.string() ).make_preferred() << " => " << ( dstPath.parent_path() / patternOut.string() ).make_preferred() );
							copy_sequence( item, patternOut, srcPath, dstPath.parent_path(), offsetStruct );
						}
						else
						{
							if(verbose)
								TUTTLE_COUT( ( item.path / item.sequence.pattern.string() ).make_preferred() << " => " << ( dstPath / item.sequence.pattern.string() ).make_preferred() );
							copy_sequence( item, item.sequence.pattern, srcPath, dstPath, offsetStruct );
						}
					}
				}
			}
			else // scan an input directory
			{
				Items inputItems = sequence::parser::browse( srcPath.string().c_str(), recursiveListing );
				BOOST_FOREACH( const sequence::BrowseItem& item, inputItems )
				{
					if( item.type == sequence::SEQUENCE )
					{
						if(verbose)
							TUTTLE_COUT( ( item.path / item.sequence.pattern.string() ).make_preferred() << " => " << ( dstPath / item.sequence.pattern.string() ).make_preferred() );
						copy_sequence( item, item.sequence.pattern, srcPath, dstPath, offsetStruct );
					}
				}
			}
		}
	}
	catch( bfs::filesystem_error &ex )
	{
		TUTTLE_COUT( _color._error << ex.what( ) << _color._std );
		return -2;
	}
	catch(...)
	{
		TUTTLE_CERR( _color._error << boost::current_exception_diagnostic_information( ) << _color._std );
		return -3;
	}
	
	return 0;
}

