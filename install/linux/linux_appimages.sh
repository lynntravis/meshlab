#!/bin/bash
# this is a script shell for setting up the AppImage bundle for linux
# Requires a properly built meshlab boundle (see linux_make_boundle.sh). It does not require to run the
# linux_deploy.sh script.
#
# This script can be run only in the oldest supported linux distro that you are using
# due to linuxdeployqt tool choice (see https://github.com/probonopd/linuxdeployqt/issues/340).
#
# Without given arguments, MeshLab AppImage will be placed in the meshlab/distrib
# directory.
#
# You can give as argument the DISTRIB_PATH.

cd "$(dirname "$(realpath "$0")")"; #move to script directory
INSTALL_PATH=$(pwd)

#checking for parameters
if [ "$#" -eq 0 ]
then
    DISTRIB_PATH=$PWD/../../distrib
else
    DISTRIB_PATH=$1
fi

cd $DISTRIB_PATH

export VERSION=$(cat $INSTALL_PATH/../../ML_VERSION)

$INSTALL_PATH/resources/linuxdeployqt usr/share/applications/meshlab_server.desktop -appimage
mv *.AppImage ../MeshLabServer$VERSION-linux.AppImage
chmod +x ../MeshLabServer$VERSION-linux.AppImage

rm AppRun 
rm *.desktop
rm *.png

#mv usr/bin/meshlabserver ..
$INSTALL_PATH/resources/linuxdeployqt usr/share/applications/meshlab.desktop -appimage
mv *.AppImage ../MeshLab$VERSION-linux.AppImage
chmod +x ../MeshLab$VERSION-linux.AppImage

chrpath -r '$ORIGIN/usr/lib:$ORIGIN/usr/lib/meshlab' AppRun

chmod +x usr/bin/meshlab
chmod +x usr/bin/meshlabserver
chmod +x AppRun

#at this point, distrib folder contains all the files necessary to execute meshlab
echo MeshLab$VERSION-linux.AppImage and MeshLabServer$VERSION-linux.AppImage generated
