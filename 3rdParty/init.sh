#!/bin/bash

# URL ARCHIVE UNCOMPRESED_ARCHIVE_DIR ALREADY_EXISTING_DIR
getAndUncompressAndTransferToDir() {
	getAndUncompress $1 $2
	mv $4/* $3
	rm -rf $4
	mv $3 $4
}

getAndUncompress() {
	echo downloading $1
	wget -nv $1&&\
	uncompress $2&&\
	echo erasing file $2&&\
	rm $2
	echo finished
}

uncompress(){
	EXTENSION=`echo $1|awk -F . '{print $NF}'`
	echo uncompressing file $1
	[ "$EXTENSION" == "bz2" ]&&tar xfj $1&&return 0
	[ "$EXTENSION" == "gz" ]&&tar xfz $1&&return 0
	return 1
}

EXPAT_FILENAME=expat-2.0.1
EXPAT_ARCHIVE=$EXPAT_FILENAME.tar.gz
EXPAT_URL=http://prdownloads.sourceforge.net/expat/$EXPAT_ARCHIVE?download
echo ">>> GETTING EXPAT"
getAndUncompressAndTransferToDir $EXPAT_URL $EXPAT_ARCHIVE $EXPAT_FILENAME expat

ZLIB_FILENAME=zlib-1.2.3
ZLIB_ARCHIVE=$ZLIB_FILENAME.tar.gz
ZLIB_URL=http://prdownloads.sourceforge.net/libpng/$ZLIB_ARCHIVE?download
echo ">>> GETTING ZLIB"
getAndUncompressAndTransferToDir $ZLIB_URL $ZLIB_ARCHIVE $ZLIB_FILENAME zlib

PNG_FILENAME=libpng-1.2.41
PNG_ARCHIVE=$PNG_FILENAME.tar.gz
PNG_URL=http://prdownloads.sourceforge.net/libpng/$PNG_ARCHIVE?download
echo ">>> GETTING PNG"
getAndUncompressAndTransferToDir $PNG_URL $PNG_ARCHIVE $PNG_FILENAME libpng

ILMBASE_FILENAME=ilmbase-1.0.1
ILMBASE_ARCHIVE=$ILMBASE_FILENAME.tar.gz
ILMBASE_URL=http://download.savannah.nongnu.org/releases/openexr/$ILMBASE_ARCHIVE
echo ">>> GETTING ILMBASE"
getAndUncompressAndTransferToDir $ILMBASE_URL $ILMBASE_ARCHIVE $ILMBASE_FILENAME ilmbase

OPENEXR_FILENAME=openexr-1.6.1
OPENEXR_ARCHIVE=$OPENEXR_FILENAME.tar.gz
OPENEXR_URL=http://download.savannah.nongnu.org/releases/openexr/$OPENEXR_ARCHIVE
echo ">>> GETTING OPENEXR"
getAndUncompressAndTransferToDir $OPENEXR_URL $OPENEXR_ARCHIVE $OPENEXR_FILENAME openexr

BOOST_FILENAME=boost_1_41_0
BOOST_ARCHIVE=$BOOST_FILENAME.tar.bz2
BOOST_URL=http://sourceforge.net/projects/boost/files/boost/1.41.0/$BOOST_ARCHIVE/download
echo ">>> GETTING Boost"
getAndUncompress $BOOST_URL $BOOST_ARCHIVE
ln -s $BOOST_FILENAME boost
echo ">>> PATCHING Boost Unit Testing Framework for $BOOST_FILENAME"
patch boost/libs/test/build/Jamfile.v2 boost_unit_testing_framework.patch

echo ">>> BUILDING Boost JAM"
CURRENT_DIR=`pwd`
cd $CURRENT_DIR/boost
source `pwd`/bootstrap.sh
cd $CURRENT_DIR

echo ">>> PREPARING bjam.sh script"
BJAM=../bjam.sh
rm -f $BJAM
echo "#!/bin/bash">>$BJAM
echo "# This script is autogenerated from 3rdParty/init.sh">>$BJAM
echo "export BOOST_BUILD_PATH=`pwd`/boost/tools/build/v2">>$BJAM
echo "`pwd`/boost/bjam \$*">>$BJAM
chmod u+x $BJAM
