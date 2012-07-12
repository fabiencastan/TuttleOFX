#ifndef TUTTLEOFX_SAM_APPLICATIONS_COMMON_FILTER_HPP
#define TUTTLEOFX_SAM_APPLICATIONS_COMMON_FILTER_HPP

#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <vector>

namespace bfs = boost::filesystem;

namespace sam
{
	bool isDotFilename( const bfs::path& p )
	{
		return p.filename().string().at(0) == '.';
	}
	
	boost::regex convertFilterToRegex( std::string filter )
	{
		boost::cmatch match;
		boost::regex expression( "(.*[%])([0-9]{2})([d].*)" ); // match to pattern like : %04d
		if( boost::regex_match( filter.c_str(), match, expression ) )
		{
			std::string matched = match[1].second;
			matched.erase( 2 , matched.size()-2); // keep only numbers
			const int patternWidth = boost::lexical_cast<int>( matched );
			std::string replacing( patternWidth, '#' );
			filter = boost::regex_replace( filter, boost::regex( "\\%\\d{1,2}d" ), replacing );
		}
	
		filter = boost::regex_replace( filter, boost::regex( "\\*" ), "(.*)" );
		filter = boost::regex_replace( filter, boost::regex( "\\?" ), "(.)" );
		filter = boost::regex_replace( filter, boost::regex( "\\@" ), "[0-9]+" ); // one @ correspond to one or more digits
		filter = boost::regex_replace( filter, boost::regex( "\\#" ), "[0-9]" ); // each # in pattern correspond to a digit
		return boost::regex( filter );
	}
	
	std::vector<boost::regex> convertFilterToRegex( const std::vector<std::string>& filters )
	{
		std::vector<boost::regex> res;
		BOOST_FOREACH( const std::string& filter, filters )
		{
			res.push_back( convertFilterToRegex( filter ) );
		}
		return res;
	}
	
	bool isFilteredFilename( const std::string& filename, const std::vector<boost::regex>& filters )
	{
		if( filters.size() == 0 )
			return false;
	
		BOOST_FOREACH( const boost::regex& filter, filters )
		{
			if( boost::regex_match( filename, filter ) )
				return false;
		}
		return true;
	}
}


#endif
