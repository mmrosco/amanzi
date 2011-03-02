# -*- mode: cmake -*-

# #############################################################################
#    
# Amanzi Configuration Report
#
# #############################################################################
include(FeatureSummary)

get_property(lang_enabled GLOBAL PROPERTY ENABLED_LANGUAGES)
get_property(pack_found GLOBAL PROPERTY PACKAGES_FOUND)
get_property(pack_not_found GLOBAL PROPERTY PACKAGES_NOT_FOUND)

message(STATUS "******************************************************************************")
message(STATUS "")
message(STATUS "${PROJECT_NAME} Configuration")
message(STATUS "Version ${AMANZI_VERSION}")
message(STATUS "")

message(STATUS "System Information")
message(STATUS "\tSystem:         ${CMAKE_SYSTEM}")
message(STATUS "\tSystem Name:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "\tSystem Version: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "System Type")           
if ( APPLE )
    message(STATUS "\tMac OSX system")
endif(APPLE)    
if ( WIN32 )
    message(STATUS "\tWindows system")
endif(WIN32)    
if ( UNIX )
    message(STATUS "\tUNIX flavor system")
endif(UNIX)   
message(STATUS "")
message(STATUS "Compilers")
message(STATUS "\tEnabled Languages: ${lang_enabled}")
message(STATUS "\tC COMPILER      ${CMAKE_C_COMPILER}")
message(STATUS "\tC COMPILER ID   ${CMAKE_C_COMPILER_ID}")
message(STATUS "\tCXX COMPILER    ${CMAKE_CXX_COMPILER}")
message(STATUS "\tCXX COMPILER ID ${CMAKE_CXX_COMPILER_ID}")
if (CMAKE_FORTRAN_COMPILER_LOADED)
    message(STATUS "\tFortran Compiler ${CMAKE_FORTRAN_COMPILER}")
    message(STATUS "\tFortran Compiler ID ${CMAKE_FORTRAN_COMPILER_ID}")
endif()    
message(STATUS "")
message(STATUS "Build type ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Compile Flags")
#message(STATUS "\tFlags:       ${CMAKE_REQUIRED_FLAGS}")
#message(STATUS "\tDefinitions: ${CMAKE_REQUIRED_DEFINITIONS}")
#message(STATUS "\tIncludes:    ${CMAKE_REQUIRED_INCLUDES}")
#message(STATUS "\tLibraries:   ${CMAKE_REQUIRED_LIBRARIES}")
#message(STATUS "")
message(STATUS  "Found packages: ${pack_found}")
foreach(pack ${pack_found})
    message(STATUS "\t${pack}")
    message(STATUS "\t\t${pack}_INCLUDE_DIRS=${${pack}_INCLUDE_DIRS}")
    message(STATUS "\t\t${pack}_INCLUDE_DIR=${${pack}_INCLUDE_DIR}")
    message(STATUS "\t\t${pack}_LIBRARY=${${pack}_LIBRARY}")
    message(STATUS "\t\t${pack}_LIBRARIES=${${pack}_LIBRARIES}")
    message(STATUS "\t\t${pack}_LIBRARY_DIR=${${pack}_LIBRARY_DIR}")
    message(STATUS "")
endforeach()    
message(STATUS  "Did not find packages: ${pack_not_found}")
message(STATUS "")
message(STATUS "Enabled mesh frameworks")
message(STATUS "\tENABLE_MOAB_Mesh   ${ENABLE_MOAB_Mesh}")
message(STATUS "\tENABLE_STK_Mesh    ${ENABLE_STK_Mesh}")
message(STATUS "\tENABLE_MSTK_Mesh   ${ENABLE_MSTK_Mesh}")
message(STATUS "")
message(STATUS "******************************************************************************")

#print_enabled_features()
#print_disabled_features()


