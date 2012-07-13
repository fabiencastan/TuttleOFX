#include <sam/common/utility.hpp>
#include <sam/common/color.hpp>
#include <sam/common/options.hpp>
#include <sam/common/filter.hpp>

#include <tuttle/common/exceptions.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

#include <sequence/parser/Browser.h>
#include <sequence/DisplayUtils.h>

#include <algorithm>
#include <iostream>
#include <iterator>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace bal = boost::algorithm;
namespace ttl = tuttle::common;
using namespace tuttle::common;

typedef std::vector<sequence::BrowseItem> Items;

bool    enableColor    = false;
bool    verbose        = false;
bool    selectRange    = false;
ssize_t firstImage     = 0;
ssize_t lastImage      = 0;

namespace sam
{
	Color _color;
	bool wasSthgDumped = false;
	
	bool sortBrowseItem ( sequence::BrowseItem i,sequence::BrowseItem j ) { return (i.path.string().length() > j.path.string().length() ); }
}

// A helper function to simplify the main part.
template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
	copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " "));
	return os;
}

void removeFiles( Items &items, const std::vector<boost::regex> reFilters, bool removeFolder, bool removeUnitFile, bool removeSequence, bool removeDotFile )
{
	for( Items::iterator item = items.begin(); item != items.end(); item++ )
	{
		bfs::path p = (*item).path;
		
		switch( (*item).type )
		{
			case sequence::FOLDER:
			{
				if( removeFolder && ( removeDotFile || !sam::isDotFilename( p ) ) && ! sam::isFilteredFilename( p.string(), reFilters ) )
				{
					if( verbose )
						TUTTLE_COUT( sam::_color._blue << p.make_preferred() << sam::_color._std );
					bfs::remove( p );
				}
				break;
			}
			case sequence::UNITFILE:
			{
				if( removeUnitFile && ( removeDotFile || !sam::isDotFilename( p ) ) && ! sam::isFilteredFilename( p.string(), reFilters ) )
				{
					if( verbose )
						TUTTLE_COUT( sam::_color._green << p.make_preferred() << sam::_color._std );
					bfs::remove( p );
				}
				break;
			}
			case sequence::SEQUENCE:
			{
				const sequence::Sequence &sequence = (*item).sequence;
				if( removeSequence && ( removeDotFile || !sam::isDotFilename( sequence.pattern.string() ) ) && ! sam::isFilteredFilename( (p / sequence.pattern.string()).string(), reFilters ) )
				{
					if( verbose )
						TUTTLE_COUT( sam::_color._green << (p / sequence.pattern.string()).make_preferred() << sam::_color._std );
					
					for( size_t index = sequence.range.first; index <= sequence.range.last; index += sequence.step )
					{
						bfs::path filename = p / sequence::instanciatePattern( sequence.pattern, index );
						bfs::remove( filename );
					}
					
				}
				break;
			}
			case sequence::UNDEFINED:
			default:
			{
				std::cout << sam::_color._red << "error: undefined " << p.string() << sam::_color._std << std::endl;
				break;
			}
		}
	}
}


int main( int argc, char** argv )
{
	using namespace tuttle::common;
	using namespace sam;

	bool recursiveListing    = false;
	bool removeUnitFile      = false;
	bool removeFolder        = false;
	bool removeSequences     = true;
	bool removeDotFile       = false; // file starting with a dot (.filename)
	bool printAbsolutePath   = false;
	std::vector<std::string> paths;
	std::vector<std::string> filters;

	// Declare the supported options.
	bpo::options_description mainOptions;
	mainOptions.add_options()
			( kAllOptionString         , kAllOptionMessage )
			( kDirectoriesOptionString , kDirectoriesOptionMessage )
			( kExpressionOptionString  , bpo::value<std::string>(), kExpressionOptionMessage )
			( kFilesOptionString       , kFilesOptionMessage )
			( kHelpOptionString        , kHelpOptionMessage )
			( kIgnoreOptionString      , kIgnoreOptionMessage )
			( kPathOptionString        , kPathOptionMessage )
			( kRecursiveOptionString   , kRecursiveOptionMessage )
			( kVerboseOptionString     , kVerboseOptionMessage )
			( kColorOptionString       , kColorOptionMessage )
			( kFirstImageOptionString  , bpo::value<std::ssize_t>(), kFirstImageOptionMessage )
			( kLastImageOptionString   , bpo::value<std::ssize_t>(), kLastImageOptionMessage )
			( kFullRMPathOptionString  , kFullRMPathOptionMessage )
			( kBriefOptionString       , kBriefOptionMessage )
			;
	
	// describe hidden options
	bpo::options_description hidden;
	hidden.add_options()
			( kInputDirOptionString, bpo::value< std::vector<std::string> >(), kInputDirOptionMessage )
			( kEnableColorOptionString, bpo::value<std::string>(), kEnableColorOptionMessage )
			;
	
	// define default options 
	bpo::positional_options_description pod;
	pod.add( kInputDirOptionString, -1 );
	
	bpo::options_description cmdline_options;
	cmdline_options.add( mainOptions ).add( hidden );

	bpo::positional_options_description pd;
	pd.add( "", -1 );
	
	//parse the command line, and put the result in vm
	bpo::variables_map vm;

	bpo::notify( vm );

	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(pod).run(), vm);

		// get environment options and parse them
		if( const char* env_rm_options = std::getenv("SAM_RM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_rm_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
		if( const char* env_rm_options = std::getenv("SAM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_rm_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
	}
	catch( const bpo::error& e )
	{
		TUTTLE_COUT( "sam-rm: command line error: " << e.what() );
		exit( -2 );
	}
	catch( ... )
	{
		TUTTLE_COUT( "sam-rm: unknown error in command line." );
		exit( -2 );
	}

	if ( vm.count( kColorOptionLongName ) )
	{
		enableColor = true;
	}
	if ( vm.count( kEnableColorOptionLongName ) )
	{
		const std::string str = vm[kEnableColorOptionLongName].as<std::string>();
		enableColor = string_to_boolean( str );
	}

	if( enableColor )
	{
		_color.enable();
	}

	if( vm.count( kHelpOptionLongName ) )
	{
		TUTTLE_COUT( _color._blue  << "TuttleOFX project [http://sites.google.com/site/tuttleofx]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "NAME" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-rm - remove file sequences" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "SYNOPSIS" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-rm [options] [sequence_pattern]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "DESCRIPTION" << _color._std << std::endl );
		TUTTLE_COUT( "Remove sequence of files, and could remove trees (folder, files and sequences)." << std::endl );
		TUTTLE_COUT( _color._blue  << "OPTIONS" << _color._std << std::endl );
		TUTTLE_COUT( mainOptions );

		TUTTLE_COUT( _color._blue << "EXAMPLES" << _color._std << std::left );
		SAM_EXAMPLE_TITLE_COUT( "Sequence possible definitions: " );
		SAM_EXAMPLE_LINE_COUT ( "Auto-detect padding : ", "seq.@.jpg" );
		SAM_EXAMPLE_LINE_COUT ( "Padding of 8 (usual style): ", "seq.########.jpg" );
		SAM_EXAMPLE_LINE_COUT ( "Padding of 8 (printf style): ", "seq.%08d.jpg" );
		SAM_EXAMPLE_TITLE_COUT( "Delete: " );
		SAM_EXAMPLE_LINE_COUT ( "A sequence:", "sam-rm /path/to/sequence/seq.@.jpg" );
		SAM_EXAMPLE_LINE_COUT ( "Sequences in a directory:", "sam-rm /path/to/sequence/" );

		return 0;
	}

	if( vm.count( kBriefOptionLongName ) )
	{
		TUTTLE_COUT( _color._green << "remove file sequences" << _color._std);
		return 0;
	}

	if(vm.count( kExpressionOptionLongName ) )
	{
		bal::split( filters, vm["expression"].as<std::string>(), bal::is_any_of( "," ) );
	}

	if( vm.count( kDirectoriesOptionLongName ) )
	{
		removeFolder = true;
	}
	
	if( vm.count( kFilesOptionLongName ) )
	{
		removeUnitFile = true;
	}
	
	if( vm.count( kIgnoreOptionLongName ) )
	{
		removeSequences = false;
	}
	
	if( vm.count( kVerboseOptionLongName ) )
	{
		verbose = true;
	}

	if( vm.count( kFirstImageOptionLongName ) )
	{
		selectRange = true;
		firstImage  = vm[kFirstImageOptionLongName].as< std::ssize_t >();
	}

	if( vm.count( kLastImageOptionLongName ) )
	{
		selectRange = true;
		lastImage  = vm[kLastImageOptionLongName].as< std::ssize_t >();
	}

	if( vm.count( kFullRMPathOptionLongName ) )
	{
		removeUnitFile  = true;
		removeFolder    = true;
		removeSequences = true;
	}
	
	if( vm.count( kAllOptionLongName ) )
	{
		// add .* files
		removeDotFile = true;
	}
	
	if( vm.count( kPathOptionLongName ) )
	{
		printAbsolutePath = true;
	}
	
	// defines paths, but if no directory specify in command line, we add the current path
	if( vm.count( kInputDirOptionLongName ) )
	{
		paths = vm[kInputDirOptionLongName].as< std::vector<std::string> >();
	}
	else
	{
		TUTTLE_COUT( _color._error << "No sequence and/or directory are specified." << _color._std );
		return 1;
	}

	if (vm.count(kRecursiveOptionLongName))
	{
		recursiveListing = true;
	}
	
	std::vector<boost::regex> reFilters = convertFilterToRegex( filters );
	
	try
	{
		BOOST_FOREACH( bfs::path path, paths )
		{
			if( bfs::exists( path ) )
			{
				//TUTTLE_TCOUT( "exists :" << path.string() );
				
				if( bfs::is_directory( path ) ) // directory
				{
					Items items = sequence::parser::browse( path.string().c_str(), recursiveListing );
					// sort to have folder after files
					sort( items.begin(), items.end(), sortBrowseItem );
					
					removeFiles( items, reFilters, removeFolder, removeUnitFile, removeSequences, removeDotFile );
				}
				else // simple file
				{
					if( verbose )
						TUTTLE_COUT( sam::_color._blue << path.c_str() << sam::_color._std );
					bfs::remove( path );
				}
			}
			else
			{
				try
				{
					Items items = sequence::parser::browse( path.parent_path().c_str(), false );
					
					boost::regex expression( path.c_str() );
					
					reFilters.push_back( expression );
					removeFiles( items, reFilters, removeFolder, removeUnitFile, removeSequences, removeDotFile );
				}
				catch(... )
				{
					TUTTLE_CERR ( _color._error << "Unrecognized pattern \"" << path << "\"" << _color._std );
				}
			}
		}
	}
	catch (bfs::filesystem_error &ex)
	{
		TUTTLE_COUT( _color._error << ex.what() << _color._std );
	}
	catch(... )
	{
		TUTTLE_CERR ( _color._error << boost::current_exception_diagnostic_information() << _color._std );
	}
	return 0;
}

