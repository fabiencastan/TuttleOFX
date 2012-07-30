#ifndef _SAM_UTILITY_HPP_
#define	_SAM_UTILITY_HPP_

#include <boost/algorithm/string/predicate.hpp>

#include <string>

bool string_to_boolean( const std::string& str )
{
	return ( str == "1" || boost::iequals(str, "y") || boost::iequals(str, "yes") || boost::iequals(str, "true") );
}

std::string convertprintfStyleToUsual( const std::string& sequenceName )
{
	std::string sequence = sequenceName;
	for( size_t i = 0; i < sequence.length(); i++ )
	{
		if( sequence.at(i) == '%' )
		{
			if( sequence.at( i + 3 ) == 'd' )
			{
				std::string value = sequence;
				value.erase( i + 3, value.length() - i - 3 );
				value.erase( 0, i + 1 );
				if( isdigit( value.at( 0 ) ) && isdigit( value.at( 1 ) ) )
				{
					sequence.erase( i, 4 );
					sequence.insert( i, atoi( value.c_str() ), '#' );
				}
			}
		}
	}
	return sequence;
}

std::string removeSharpAndArobase( const std::string& sequenceName )
{
	std::string sequence = sequenceName;
	for( size_t i = 0; i < sequence.length(); i++ )
	{
		if( sequence.at(i) == '#' || sequence.at(i) == '@' )
		{
			sequence.erase( i, 1 );
			i--;
		}
	}
	return sequence;
}

bool sequenceNamesMatch( const std::string& sequenceA, const std::string& sequenceB )
{
	if( strcmp( convertprintfStyleToUsual( sequenceA ).c_str(), convertprintfStyleToUsual( sequenceB ).c_str() ) == 0 )
		return true;
	
	size_t foundA = sequenceA.find( '@' );
	size_t foundB = sequenceB.find( '@' );
	if( foundA != std::string::npos || foundB != std::string::npos )
	{
		if( strcmp( removeSharpAndArobase( sequenceA ).c_str(), removeSharpAndArobase( sequenceB ).c_str() ) == 0 )
			return true;
	}
	return false;
}


#endif

