##############################################################################
# OpenMEEG
#
# Copyright (c) INRIA 2015-2016. All rights reserved.
# See LICENSE.txt for details.
# 
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.
################################################################################

set(CPACK_BINARY_TGZ ON)

set(CPACK_BINARY_DRAGNDROP OFF)
set(CPACK_BINARY_PACKAGEMAKER OFF)

#   Get distribution name and architecture

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "PackageMaker;TGZ")

set(PACKAGE_ARCH "MacOSX-Intel")
if(BUILD_UNIVERSAL)
    set(PACKAGE_ARCH "Apple-Universal")
endif()

## #############################################################################
## Add Apple packaging script
## #############################################################################

#configure_file(${CMAKE_SOURCE_DIR}/packaging/apple/ApplePackScript.cmake.in ${PROJECT_BINARY_DIR}/tmp.in)
  
#configure_file( ${PROJECT_BINARY_DIR}/tmp.in ${PROJECT_BINARY_DIR}/packaging/apple/ApplePackScript.cmake)
#configure_file(${CMAKE_SOURCE_DIR}/packaging/apple/mac_packager.sh.in ${PROJECT_BINARY_DIR}/packaging/apple/mac_packager.sh)

#set(CPACK_INSTALL_SCRIPT ${PROJECT_BINARY_DIR}/packaging/apple/ApplePackScript.cmake)
