#!/bin/sh
#
#  gen-installer.sh - Generate a Mac OS X installer
#
#  $Id$
#
#  odbc-bench - a TPC-A and TPC-C like benchmark program for databases
#  Copyright (C) 2000-2005 OpenLink Software <odbc-bench@openlinksw.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

#
#  Where to put the generated files
#
CUR=`pwd`
DISTRIB=${DISTRIB:-$CUR/distrib}

#
#  Used utilities
#
CP=/bin/cp
RM=/bin/rm

NEWFS=/sbin/newfs_hfs
HDID=/usr/bin/hdid
HDIUTIL=/usr/bin/hdiutil
MKDIR=/bin/mkdir
SYNC=/bin/sync

XCODEBUILD=/usr/bin/xcodebuild
PKGMAKER=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker


#
#  Functions
#
check_failed() {
    if test $? -ne 0
    then
	echo
	echo "FAILED: $*"
	echo
	exit 1
    fi
}
#
#  Install the package into a temp location
#
$MKDIR -p $DISTRIB
$MKDIR -p $DISTRIB/tmp
check_failed "create $DISTRIB/tmp directory"

$XCODEBUILD install -buildstyle Deployment DSTROOT=$DISTRIB/tmp
check_failed "building/installing package"


#
#  Create package
#
$PKGMAKER -build \
	-p $DISTRIB/odbc-bench.pkg \
	-f $DISTRIB/tmp \
	-r $CUR/Resources \
	-i $CUR/Installer/Info.plist \
	-d $CUR/Installer/Description.plist 
check_failed "assembling odbc-bench.pkg"

rm -rf $DISTRIB/tmp
check_failed "removing $DISTRIB/tmp directory"

#
#  Generating disk image
#
$RM -rf $DISTRIB/ODBC-Bench-0.99.25.dmg
$RM -rf $DISTRIB/TargetImage.dmg

$HDIUTIL create -megabytes 5 $DISTRIB/TargetImage.dmg -layout NONE -fs HFS+
check_failed "creating temp disk image"
$SYNC

DEVDSK=`$HDID -nomount $DISTRIB/TargetImage.dmg`
$SYNC

$NEWFS -v 'ODBC-Bench-0.99.25' $DEVDSK
check_failed "labeling disk image"
$SYNC

$HDIUTIL eject $DEVDSK
check_failed "ejecting disk image"
$SYNC

$HDID $DISTRIB/TargetImage.dmg
check_failed "mounting disk image"
$SYNC

$CP -R -f $DISTRIB/odbc-bench.pkg /Volumes/ODBC-Bench-0.99.25/odbc-bench.pkg
check_failed "copying odbc-bench.pkg to disk image"
$SYNC

$HDIUTIL eject $DEVDSK
check_failed "ejecting disk image"
$SYNC

$HDIUTIL convert -format UDCO $DISTRIB/TargetImage.dmg -o $DISTRIB/ODBC-Bench-0.99.25.dmg
check_failed "converting disk image"

$RM -rf $DISTRIB/TargetImage.dmg
check_failed "removing temp disk image"


#
#  All Done
#
echo
echo "PASSED: Installer generation completed successfully"
echo
exit 0
