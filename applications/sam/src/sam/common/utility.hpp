#ifndef _SAM_UTILITY_HPP_
#define	_SAM_UTILITY_HPP_

#include <boost/algorithm/string/predicate.hpp>

#include <string>


bool string_to_boolean( const std::string& str )
{
	return ( str == "1" || boost::iequals(str, "y") || boost::iequals(str, "yes") || boost::iequals(str, "true") );
}

void convertprintfStyleToUsual( std::string& sequenceName )
{
	for( size_t i=0; i< sequenceName.length(); i++ )
	{
		if( sequenceName.at(i) == '%' )
		{
			if( sequenceName.at( i + 3 ) == 'd' )
			{
				std::string value = sequenceName;
				value.erase( i + 3, value.length() - i - 3 );
				value.erase( 0, i + 1 );
				if( isdigit( value.at( 0 ) ) && isdigit( value.at( 1 ) ) )
				{
					sequenceName.erase( i, 4 );
					sequenceName.insert( i, atoi( value.c_str() ), '#' );
				}
			}
		}
	}
}

#endif

