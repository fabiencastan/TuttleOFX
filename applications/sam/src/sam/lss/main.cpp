#include <sam/common/utility.hpp>
#include <sam/common/color.hpp>
#include <sam/common/options.hpp>
#include <sam/common/filter.hpp>

#include <tuttle/common/utils/global.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <sequence/parser/Browser.h>
#include <sequence/DisplayUtils.h>

#include <algorithm>
#include <iostream>
#include <iterator>

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;
namespace bal = boost::algorithm;

namespace sam
{
	Color _color;
	bool wasSthgDumped = false;	
	
	bool sortBrowseItem ( sequence::BrowseItem i,sequence::BrowseItem j )
	{
		bfs::path rootI = i.type == sequence::UNITFILE ? i.path.parent_path().string() : i.path.string() ;
		bfs::path rootJ = j.type == sequence::UNITFILE ? j.path.parent_path().string() : j.path.string() ;
		
		int res = strcmp( rootI.string().c_str(), rootJ.string().c_str() );

		if( res != 0 )
		{
			if( res > 0 )
				return false;
			else
				return true;
		}
		else
		{
			bool iIsAFolder = i.type == sequence::FOLDER;
			bool jIsAFolder = j.type == sequence::FOLDER;
			if( iIsAFolder || jIsAFolder )
			{
				if( iIsAFolder && ! jIsAFolder )
				{
					return true;
				}
				if( ! iIsAFolder && jIsAFolder )
				{
					return false;
				}
				return true;
			}
			
			std::string nameI =  i.type == sequence::SEQUENCE ? i.sequence.pattern.string().c_str() : i.path.filename().string() ;
			std::string nameJ =  j.type == sequence::SEQUENCE ? j.sequence.pattern.string().c_str() : j.path.filename().string() ;
			
			int resName = strcmp( nameI.c_str(), nameJ.c_str() );
			if( resName > 0 )
				return false;
			else
				return true;
		}
		
		return true;
	}
}

int main( int argc, char** argv )
{
	using namespace tuttle::common;
	using namespace sam;

	typedef std::vector<sequence::BrowseItem> Items;

	bool recursiveListing    = false;
	bool script              = false;
	bool enableColor         = false;
	bool listUnitFile        = false;
	bool listFolder          = false;
	bool maskSequences       = false;
	bool listDotFile         = false; // file starting with a dot (.filename)
	bool listLongListing     = false;
	bool listRelativePath    = false;
	bool listAbsolutePath    = false;
	
	//std::string       availableExtensions;
	std::vector<std::string> paths;
	std::vector<std::string> filters;

	// Declare the supported options.
	bpo::options_description mainOptions;
	mainOptions.add_options()
		(kAllOptionString          , kAllOptionMessage)
		(kDirectoriesOptionString  , kDirectoriesOptionMessage)
		(kExpressionOptionString   , bpo::value<std::string>(), kExpressionOptionMessage)
		(kFilesOptionString        , kFilesOptionMessage )
		(kHelpOptionString         , kHelpOptionMessage)
		(kLongListingOptionString  , kLongListingOptionMessage)
		(kIgnoreOptionString       , kIgnoreOptionMessage)
		(kRelativePathOptionString , kRelativePathOptionMessage)
		(kRecursiveOptionString    , kRecursiveOptionMessage)
		(kPathOptionString         , kPathOptionMessage)
		(kColorOptionString        , kColorOptionMessage)
		(kFullDisplayOptionString  , kFullDisplayOptionMessage )
		(kScriptOptionString       , kScriptOptionMessage)
		(kBriefOptionString        , kBriefOptionMessage)
	;
	
	// describe hidden options
	bpo::options_description hidden;
	hidden.add_options()
		(kInputDirOptionString, bpo::value< std::vector<std::string> >(), kInputDirOptionMessage)
		(kEnableColorOptionString, bpo::value<std::string>(), kEnableColorOptionMessage)
	;
	
	// define default options 
	bpo::positional_options_description pod;
	pod.add(kInputDirOptionLongName, -1);
	
	bpo::options_description cmdline_options;
	cmdline_options.add(mainOptions).add(hidden);

	bpo::positional_options_description pd;
	pd.add("", -1);
	
	bpo::variables_map vm;

	try
	{
		//parse the command line, and put the result in vm
		bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(pod).run(), vm);

		// get environment options and parse them
		if( const char* env_lss_options = std::getenv("SAM_LSS_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_lss_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
		if( const char* env_lss_options = std::getenv("SAM_OPTIONS") )
		{
			const std::vector<std::string> vecOptions = bpo::split_unix( env_lss_options, " " );
			bpo::store(bpo::command_line_parser(vecOptions).options(cmdline_options).positional(pod).run(), vm);
		}
		bpo::notify(vm);
	}
	catch( const bpo::error& e)
	{
		TUTTLE_COUT("sam-lss: command line error: " << e.what() );
		exit( -2 );
	}
	catch(...)
	{
		TUTTLE_COUT("sam-lss: unknown error in command line.");
		exit( -2 );
	}

	if ( vm.count(kScriptOptionLongName) )
	{
		// disable color, disable directory printing and set relative path by default
		script = true;
	}

	if ( vm.count(kColorOptionLongName) && !script )
	{
		enableColor = true;
	}
	if ( vm.count(kEnableColorOptionLongName) && !script )
	{
		const std::string str = vm[kEnableColorOptionLongName].as<std::string>();
		enableColor = string_to_boolean( str );
	}

	if( enableColor )
	{
		_color.enable();
	}

	if (vm.count(kHelpOptionLongName))
	{
		TUTTLE_COUT( _color._blue  << "TuttleOFX project [http://sites.google.com/site/tuttleofx]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "NAME" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-lss - list directory contents" << _color._std << std::endl);
		TUTTLE_COUT( _color._blue  << "SYNOPSIS" << _color._std );
		TUTTLE_COUT( _color._green << "\tsam-lss [options] [directories]" << _color._std << std::endl );
		TUTTLE_COUT( _color._blue  << "DESCRIPTION" << _color._std << std::endl );

		TUTTLE_COUT( "List information about the sequences, files and folders." );
		TUTTLE_COUT( "List the current directory by default, and only sequences." );
		TUTTLE_COUT( "The script option disable color, disable directory printing (in multi-directory case or recursive) and set relative path by default." << std::endl );

		TUTTLE_COUT( _color._blue  << "OPTIONS" << _color._std << std::endl );
		TUTTLE_COUT( mainOptions );
		return 0;
	}

	if ( vm.count(kBriefOptionLongName) )
	{
		TUTTLE_COUT( _color._green << "list directory contents" << _color._std);
		return 0;
	}

	if (vm.count(kExpressionOptionLongName))
	{
		TUTTLE_COUT( _color._red << "Expression: " << vm["expression"].as<std::string>() << _color._std );
		bal::split( filters, vm["expression"].as<std::string>(), bal::is_any_of(","));
	}

	if (vm.count(kDirectoriesOptionLongName))
	{
		listFolder = true;
	}
	
	if (vm.count(kFilesOptionLongName))
	{
		listUnitFile = true;
	}
	
	if (vm.count(kIgnoreOptionLongName))
	{
		maskSequences = true;
	}
	
	if (vm.count(kFullDisplayOptionLongName))
	{
		listFolder = true;
		listUnitFile = true;
		maskSequences = false;
	}

	if (vm.count(kAllOptionLongName))
	{
		// add .* files
		listDotFile = true;
	}
	
	if (vm.count(kLongListingOptionLongName))
	{
		listLongListing = true;
	}
	
	if (vm.count(kRelativePathOptionLongName) )
	{
		listRelativePath = true;
	}

	if(vm.count(kPathOptionLongName))
	{
		listAbsolutePath = true;
	}
	
	// defines paths, but if no directory specify in command line, we add the current path
	if (vm.count(kInputDirOptionLongName))
	{
		paths = vm[kInputDirOptionLongName].as< std::vector<std::string> >();
	}
	else
	{
		paths.push_back( "./" );
	}
	
	if (vm.count(kRecursiveOptionLongName))
	{
		recursiveListing = true;
	}
	
	// build filter into regex expression
	const std::vector<boost::regex> reFilters = convertFilterToRegex( filters );
	const bool listSinglePath = paths.size() == 1;
	
	try
	{
		BOOST_FOREACH( bfs::path path, paths )
		{
			TUTTLE_TCOUT_VAR( path.string().c_str() );
			Items items;
			try
			{
				 items = sequence::parser::browse( path.string().c_str(), recursiveListing );
			}
			catch ( const std::exception& ex)
			{
				if( path.has_parent_path() )
				{
					std::string filename = path.filename().string();
					filters.push_back( filename);
					bfs::path root = path.parent_path();
					TUTTLE_TCOUT_VAR2( path.string(), filename );
					items = sequence::parser::browse( root.string().c_str(), false );
					
					Items newItems;
					
					for( Items::iterator item = items.begin(); item != items.end(); item++ )
					{
						switch( (*item).type )
						{
							case sequence::FOLDER:
							case sequence::UNITFILE:
							{
								//TUTTLE_COUT( (*item).path.string() << "-\t-" << path.string()<< "-" );
								if( strcmp( (*item).path.string().c_str(), path.string().c_str() ) == 0 )
								{
									newItems.push_back( *item );
								}
								break;
							}
							case sequence::SEQUENCE:
							{
								const sequence::Sequence &sequence = (*item).sequence;
								//TUTTLE_TCOUT_VAR3( sequence.pattern.string().c_str(), filename.c_str(), path.string().c_str() );
								if( strcmp( sequence.pattern.string().c_str(), filename.c_str() ) == 0 )
								{
									newItems.push_back( *item );
								}
								break;
							}
							case sequence::UNDEFINED:
							{
								break;
							}
						}
					}

					if( newItems.size() == 0 )
					{
						TUTTLE_CERR( _color._error << "No such file or directory with this name." << _color._std );
						return -1;
					}
					
					path = root;
					items.clear();
					items = newItems;
				}
			}

			sort( items.begin(), items.end(), sortBrowseItem );

			std::string f = path.make_preferred().string();
			
			size_t removeInitialPath = f.length();
			
			if( strcmp( path.make_preferred().string().c_str(), "/" ) == 0 )
				removeInitialPath = 0;
			
			if( path.make_preferred().string().at( f.length() - 1 ) != '/' )
				removeInitialPath += 1;
			
			if( !listSinglePath )
				TUTTLE_COUT( path.string() << ":" );
			
			for( Items::iterator item = items.begin(); item != items.end(); item++ )
			{
				bfs::path p = (*item).path;
				std::string itemPath = (*item).path.make_preferred().string();
				
				if( removeInitialPath >= itemPath.length() )
					itemPath.clear();
				else
					itemPath.erase( itemPath.begin(), itemPath.begin() + removeInitialPath );
				
				//std::cout << removeInitialPath << "  " << p.string().c_str() << " => " << itemPath << "."<< std::endl;
				if( listRelativePath )
				{
					p = p.relative_path( );
					itemPath = p.string();
				}
				if( listAbsolutePath )
				{
					p = bfs::absolute( p );
					itemPath = itemPath = p.string();
				}
				
				switch( (*item).type )
				{
					case sequence::FOLDER:
					{
						if( ( recursiveListing && ! listAbsolutePath ) || ( listFolder && ( listDotFile || !isDotFilename( p ) ) && ! isFilteredFilename( p.string(), reFilters ) ) )
						{
							TUTTLE_COUT( ( listLongListing ? "d " : "") << _color._blue << itemPath << "/" << _color._std );
							wasSthgDumped = true;
						}
						break;
					}
					case sequence::UNITFILE:
					{
						if( listUnitFile && ( listDotFile || !isDotFilename( p ) ) && ! isFilteredFilename( p.string(), reFilters ) )
						{
							bfs::path file( itemPath );
							std::string filename = file.string();
							if( ! listRelativePath && ! listAbsolutePath )
								filename = file.filename().string();
							
							TUTTLE_COUT( ( listLongListing ? "f " : "" ) << _color._green << filename << _color._std );
							wasSthgDumped = true;
						}
						break;
					}
					case sequence::SEQUENCE:
					{
						const sequence::Sequence &sequence = (*item).sequence;
						if( !maskSequences && ( listDotFile || !isDotFilename( sequence.pattern.string() ) ) && ! isFilteredFilename( (p / sequence.pattern.string()).string(), reFilters ) )
						{
							std::string sequenceName;
							if( itemPath.length() != 0 && ( listAbsolutePath || listRelativePath )  )
							{
								sequenceName += itemPath;
								sequenceName += "/";
							}
							sequenceName += sequence.pattern.string();
							
							std::cout << ( listLongListing ? "s " : "" ) << _color._green << sequenceName << _color._std;
							std::cout << ' ' << sequence.range;
							if (sequence.step > 1)
								std::cout << " (" << sequence.step << ')';
							std::cout << std::endl;
							wasSthgDumped = true;
						}
						break;
					}
					case sequence::UNDEFINED:
					{
						TUTTLE_COUT( ( listLongListing ? "u " : "" ) << p.string() );
						wasSthgDumped = true;
						break;
					}
				}
			}
		}
	}
	catch ( const bfs::filesystem_error& ex)
	{
		TUTTLE_COUT( ex.what() );
	}
	catch( ... )
	{
		TUTTLE_CERR ( boost::current_exception_diagnostic_information() );
	}
	if(!wasSthgDumped)
		TUTTLE_CERR ( _color._error << "No sequence found here." << _color._std );
	return 0;
}
