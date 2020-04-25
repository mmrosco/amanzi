#  -*- mode: cmake -*-

#
# Build TPL: ASCEMIO 
#  
# --- Define all the directories and common external project flags
define_external_project_args(ASCEMIO 
                             TARGET ascemio
                             DEPENDS HDF5 ZLIB)

# add ASCEMIO version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX ASCEMIO
  VERSION ${ASCEMIO_VERSION_MAJOR} ${ASCEMIO_VERSION_MINOR} ${ASCEMIO_VERSION_PATCH})  

# --- Patch the original code
set(ASCEMIO_patch_file ascemio-2.2-sprintf.patch
                       ascemio-2.2-CMakeLists.txt.patch
                       ascemio-2.2-hdf5.patch)
set(ASCEMIO_sh_patch ${ASCEMIO_prefix_dir}/ascemio-patch-step.sh)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/ascemio-patch-step.sh.in
               ${ASCEMIO_sh_patch}
               @ONLY)
# configure the CMake patch step
set(ASCEMIO_cmake_patch ${ASCEMIO_prefix_dir}/ascemio-patch-step.cmake)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/ascemio-patch-step.cmake.in
               ${ASCEMIO_cmake_patch}
               @ONLY)
# set the patch command
set(ASCEMIO_PATCH_COMMAND ${CMAKE_COMMAND} -P ${ASCEMIO_cmake_patch})     

# --- Define the arguments passed to CMake.
set(ASCEMIO_CMAKE_ARGS 
      "-DCMAKE_INSTALL_PREFIX:FILEPATH=${TPL_INSTALL_PREFIX}"
      "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
      "-DHDF5_DIR=${HDF5_DIR}")

if (BUILD_SHARED_LIBS)
  set(HDF5_USE_STATIC_LIBRARIES OFF)
else()
  set(HDF5_USE_STATIC_LIBRARIES ON)
endif()

# --- Add external project build and tie to the ZLIB build target
ExternalProject_Add(${ASCEMIO_BUILD_TARGET}
                    DEPENDS   ${ASCEMIO_PACKAGE_DEPENDS}          # Package dependency target
                    TMP_DIR   ${ASCEMIO_tmp_dir}                  # Temporary files directory
                    STAMP_DIR ${ASCEMIO_stamp_dir}                # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}              # Download directory
                    URL          ${ASCEMIO_URL}                   # URL may be a web site OR a local file
                    URL_MD5      ${ASCEMIO_MD5_SUM}               # md5sum of the archive file
                    # -- Patch 
                    PATCH_COMMAND ${ASCEMIO_PATCH_COMMAND}
                    # -- Configure
                    SOURCE_DIR  ${ASCEMIO_source_dir} 
                    CMAKE_ARGS  ${AMANZI_CMAKE_CACHE_ARGS}        # Ensure uniform build
                                ${ASCEMIO_CMAKE_ARGS}
                                -DCMAKE_C_FLAGS:STRING=${Amanzi_COMMON_CFLAGS}   # Ensure uniform build
                                -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                -DHDF5_USE_STATIC_LIBRARIES:BOOL=${HDF5_USE_STATIC_LIBRARIES}
                    # -- Build
                    BINARY_DIR       ${ASCEMIO_build_dir}         # Build directory 
                    BUILD_COMMAND    ${MAKE}
                    BUILD_IN_SOURCE  FALSE                        # Flag for in source builds
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}        # Install directory
                    INSTALL_COMMAND  $(MAKE) install
                    # -- Output control
                    ${ASCEMIO_logging_args})


include(BuildLibraryName)
build_library_name(ascemio ASCEMIO_LIB APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)
set(ASCEMIO_INCLUDE_DIRS ${TPL_INSTALL_PREFIX})
set(ASCEMIO_DIR ${TPL_INSTALL_PREFIX})
