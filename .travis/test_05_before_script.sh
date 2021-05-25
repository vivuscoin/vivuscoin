#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

DOCKER_EXEC echo \> \$HOME/.vivuscoin  # Make sure default datadir does not exist and is never read by creating a dummy file

mkdir -p depends/SDKs depends/sdk-sources

if [ -n "$OSX_SDK" -a ! -f depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  echo "$SDK_URL/MacOSX${OSX_SDK}.sdk.tar.gz"
  curl --location --fail $SDK_URL/MacOSX${OSX_SDK}.sdk.tar.gz -o depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [ -n "$OSX_SDK" -a -f depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz ]; then
  tar -C depends/SDKs -xf depends/sdk-sources/MacOSX${OSX_SDK}.sdk.tar.gz
fi
if [[ $HOST = *-mingw32 ]]; then
  DOCKER_EXEC update-alternatives --set $HOST-g++ \$\(which $HOST-g++-posix\)
fi
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC CONFIG_SHELL= make $MAKEJOBS -C depends HOST=$HOST $DEP_OPTS
fi

touch $HOME/.ccache/client_version_build
N=$(cat $HOME/.ccache/client_version_build)
sed -i "s/define(_CLIENT_VERSION_BUILD.*/define(_CLIENT_VERSION_BUILD, $((N+1)))/" configure.ac
echo $((N+1)) > $HOME/.ccache/client_version_build

sed -i "1s/^/#define GIT_COMMIT_ID \"$(git rev-parse --short HEAD)\"\n/" src/clientversion.cpp
CLIENT_VERSION_MAJOR=$(grep "define(_CLIENT_VERSION_MAJOR" configure.ac|cut -d' ' -f2|cut -d')' -f1)
CLIENT_VERSION_MINOR=$(grep "define(_CLIENT_VERSION_MINOR" configure.ac|cut -d' ' -f2|cut -d')' -f1)
CLIENT_VERSION_REVISION=$(grep "define(_CLIENT_VERSION_REVISION" configure.ac|cut -d' ' -f2|cut -d')' -f1)
CLIENT_VERSION_BUILD=$(grep "define(_CLIENT_VERSION_BUILD" configure.ac|cut -d' ' -f2|cut -d')' -f1)
export CLIENT_VERSION="$CLIENT_VERSION_MAJOR.$CLIENT_VERSION_MINOR.$CLIENT_VERSION_REVISION.$CLIENT_VERSION_BUILD"

export SHA_DMG=$(sha256sum Vivuscoin-Core.dmg)
