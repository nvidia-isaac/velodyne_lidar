#!/bin/bash
#####################################################################################
# Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#####################################################################################

# contants
BAZEL_BIN="bazel-bin"

# used arguments with default values
UNAME=$USER
REMOTE_USER=nvidia

# get command line arguments
while [ $# -gt 0 ]; do
  case "$1" in
    -p|--package)
      PACKAGE="$2"
      ;;
    -d|--device)
      DEVICE="$2"
      ;;
    -h|--host)
      HOST="$2"
      ;;
    -u|--user)
      UNAME="$2"
      ;;
    -s|--symbols)
      NEED_SYMBOLS="True"
      shift
      continue
      ;;
    -r|--run)
      NEED_RUN="True"
      shift
      continue
      ;;
    --remote_user)
      REMOTE_USER="$2"
      ;;
    --deploy_path)
      DEPLOY_PATH="$2"
      ;;
    *)
      printf "Error: Invalid arguments: %1 %2\n"
      exit 1
  esac
  shift
  shift
done

if [ -z "$PACKAGE" ]; then
  echo "Error: Package must be specified with -p //foo/bar:tar."
  exit 1
fi
if [[ $PACKAGE != //* ]]; then
  echo "Error: Package must start with //. For example: //foo/bar:tar."
  exit 1
fi

if [ -z "$HOST" ]; then
  echo "Error: Host IP must be specified with -h IP."
  exit 1
fi

if [ -z "$DEVICE" ]; then
  echo "Error: Desired target device must be specified with -d DEVICE. Valid choices: 'jetpack42', 'x86_64'."
  exit 1
fi

# Split the target of the form //foo/bar:tar into "//foo/bar" and "tar"
targetSplitted=(${PACKAGE//:/ })
if [[ ${#targetSplitted[@]} != 2 ]]; then
  echo "Error: Package '$PACKAGE' must have the form //foo/bar:tar"
  exit 1
fi
PREFIX=${targetSplitted[0]:2}
TARGET=${targetSplitted[1]}

# check if multiple potential output files are present and if so delete them first
TAR1="$BAZEL_BIN/$PREFIX/$TARGET.tar"
TAR2="$BAZEL_BIN/$PREFIX/$TARGET.tar.gz"
if [[ -f $TAR1 ]] && [[ -f $TAR2 ]]; then
  rm -f $TAR1
  rm -f $TAR2
fi

# build the bazel package
echo "================================================================================"
echo "Building //$PREFIX:$TARGET for target platform '$DEVICE'"
echo "================================================================================"
bazel build --strip=always --config $DEVICE $PREFIX:$TARGET || exit 1

# Find the filename of the tar archive. We don't know the filename extension so we look for the most
# recent file and take the corresponding extension. We accept .tar or .tar.gz extensions.
if [[ -f $TAR1 ]] && [[ $TAR1 -nt $TAR2 ]]; then
  EX="tar"
elif [[ -f $TAR2 ]] && [[ $TAR2 -nt $TAR1 ]]; then
  EX="tar.gz"
else
  echo "Error: Package '$PACKAGE' did not produce a .tar or .tar.gz file"
  exit 1
fi
TAR="$TARGET.$EX"

# Print a message with the information we gathered so far
echo "================================================================================"
echo "Deploying //$PREFIX:$TARGET ($EX) to $REMOTE_USER@$HOST under name '$UNAME'"
echo "================================================================================"

# unpack the package in the local tmp folder
rm -f /tmp/$TAR
cp $BAZEL_BIN/$PREFIX/$TAR /tmp/
rm -rf /tmp/$TARGET
mkdir /tmp/$TARGET
tar -xf /tmp/$TAR -C /tmp/$TARGET

# Deploy directory
if [ -z "$DEPLOY_PATH" ]
then
  DEPLOY_PATH="/home/$REMOTE_USER/deploy/$UNAME/"
fi

# sync the package folder to the remote
rsync -avz --delete --checksum --rsync-path="mkdir -p $DEPLOY_PATH/ && rsync" \
    /tmp/$TARGET $REMOTE_USER@$HOST:$DEPLOY_PATH
echo "================================================================================"
echo "Done"
echo "================================================================================"

if [[ ! -z $NEED_RUN ]]; then
  echo "================================================================================"
  echo "Running on Remote"
  echo "================================================================================"
  # echo "cd $DEPLOY_PATH/$TARGET; ./$PREFIX/${TARGET::-4}"
  ssh -t $REMOTE_USER@$HOST "cd $DEPLOY_PATH/$TARGET; ./$PREFIX/${TARGET::-4}"
fi
