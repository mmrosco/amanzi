# -*- mode: cmake -*-
# 
# Amanzi Third Party Library (TPL) Definitions
#

# Standard CMake modules see CMAKE_ROOT/Modules
include(CMakeDependentOption)
include(FeatureSummary)

# Amanzi CMake modules see <root source>/tools/cmake
include(PrintVariable)

##############################################################################
# Boost
##############################################################################
find_package( Boost COMPONENTS system filesystem program_options REQUIRED)
set_feature_info(Boost
                 "C++ Extension library"
                 "http://www.boost.org"
                 "Required by the MPC")


##############################################################################
# HDF5 - http://www.hdfgroup.org/HDF5/
##############################################################################
find_package(HDF5 REQUIRED)
if ( NOT HDF5_IS_PARALLEL ) 
    message(FATAL_ERROR "The HDF5 installation found in ${HDF5_DIR} is not\n"
                        "a parallel build. Please re-run cmake and define\n"
                        "a HDF5 installation that is parallel.\n")
endif(NOT HDF5_IS_PARALLEL)
set_feature_info(HDF5
                "I/O library that creates HDF5 formatted files"
                "http://www.hdfgroup.org/HDF5"
                "Required library for several components in Amanzi"
                )

##############################################################################
# Trilinos http://trilinos.sandia.gov
##############################################################################
# This command alters Trilinos_DIR. If it finds the configuration file
# Trilinos_DIR is set to the path the configuration file was found.
find_package(Trilinos 10.6 REQUIRED
             HINTS ${Trilinos_DIR}
             PATH_SUFFIXES include)
            
if ( Trilinos_FOUND )

    message(STATUS "Found Trilinos: ${Trilinos_LIBRARY_DIR}")

    # Amanzi uses Epetra and Teuchos utils throughout the code. 
    # This find_package call defines Epetra_* variables.
    # Amanzi developers should use these variables
    # for libraries that ONLY use Epetra/Teuchos and avoid
    # using the ALL POWERFUL(TM) Trilinos_LIBRARIES.
    # When/If we create wrappers, using this variable
    # will make that transition easier.
    find_package(Epetra
                 NO_MODULE
                 HINTS ${Trilinos_DIR}
                 PATH_SUFFIXES include
                 )
                
    find_package(Teuchos
                 NO_MODULE
                 HINTS ${Trilinos_DIR}
                 PATH_SUFFIXES include
                 )
                
    # STK (Mesh framework) is not a default Trilinos package
    # Must explicity build this package. This is a check to see
    # if STK is in Trilinos. 
    find_package(STK 
                 NO_MODULE
                 HINTS ${Trilinos_DIR}
                 PATH_SUFFIXES include
                 )

    # For some reason, Trilinos defines dependent TPLs in *_TPL_LIBRARIES not
    # in *_LIBRARIES. We update the variables so the usage of these variables
    # is consistent with other FindXXX modules.
    list(APPEND Epetra_LIBRARIES "${Epetra_TPL_LIBRARIES}")
    list(APPEND Epetra_INCLUDE_DIRS "${Epetra_TPL_INCLUDE_DIRS}")
    list(APPEND Teuchos_LIBRARIES "${Teuchos_TPL_LIBRARIES}")
    list(APPEND Teuchos_INCLUDE_DIRS "${Teuchos_TPL_INCLUDE_DIRS}")
    list(APPEND STK_LIBRARIES "${STK_TPL_LIBRARIES}")
    list(APPEND STK_INCLUDE_DIRS "${STK_TPL_INCLUDE_DIRS}")

    list(APPEND Trilinos_LIBRARIES "${Trilinos_TPL_LIBRARIES}")
    list(APPEND Trilinos_INCLUDE_DIRS "${Trilinos_TPL_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Can not locate Trilinos configuration file\n"
                        " Please define the location of your Trilinos installation\n"
                        "using -D Trilinos_DIR:FILEPATH=<install path>\n")
endif()    

# Trilinos can contain 20 or more libraries (packages). The variable Trilino_LIBRARIES
# does not have full path names only library names. I suspect if it did include
# full path names the link command would exceed the command line character limit on
# many platforms, thus we need to add Trilinos library path to the link to find these
# libraries. Since Amanzi calls many Trilinos packages directly we'll add Trilinos
# to all link commands here. Yes, this is overkill. We'll have wrappers someday.
link_directories(${Trilinos_LIBRARY_DIRS})



##############################################################################
# NetCDF - http://www.unidata.ucar.edu/software/netcdf/
##############################################################################
find_package(NetCDF REQUIRED)
set_feature_info(NetCDF
                 "Network Common Data Format (NetCDF)"
                 "http://www.unidata.ucar.edu/software/netcdf/"
                 "Required by ExodusII library")

##############################################################################
# Exodus II -http://sourceforge.net/projects/exodusii
##############################################################################
find_package(ExodusII REQUIRED)
set_feature_info(ExodusII
                 "File format library. Originated from Sandia."
                 "http://sourceforge.net/projects/exodusii/"
                 "Required by all the mesh frameworks to read mesh files")


# Enabled TPLs

# Mesh TPLs

# Enable ALL possible mesh frameworks
cmake_dependent_option(ENABLE_ALL_Mesh "Build all Amanzi mesh frameworks" OFF
                       "ENABLE_STK_Mesh;ENABLE_MOAB_Mesh;ENABLE_MSTK" ON) 
set_feature_info(ALL_Mesh
                 ENABLE_ALL_Mesh
                 "Build all available mesh frameworks"
                  )    

##############################################################################
# STK - Sierra Mesh Tool Kit part of Trilinos
##############################################################################
option(ENABLE_STK_Mesh  "Build Amanzi with the STK mesh framework" ON)
set_feature_info(STK_Mesh
                 ENABLE_STK_Mesh
                 "Sierra Mesh Tool Kit (STK Mesh) a Trilinos package"
                 )

##############################################################################
# MOAB - svn co https://svn.mcs.anl.gov/repos/ITAPS/MOAB/trunk MOAB
##############################################################################
option(ENABLE_MOAB_Mesh "Build Amanzi with the MOAB mesh framework" OFF)
set_feature_info(MOAB_Mesh
                 ENABLE_MOAB_Mesh
                 "A Mesh-Oriented datABase"
                 )
if (ENABLE_MOAB_Mesh)
    find_package(MOAB REQUIRED)
endif()

##############################################################################
# MSTK - https://software.lanl.gov/MeshTools/trac/raw-attachment/wiki/WikiStart/mstk-1.80.tar.gz
##############################################################################
#PRINT_VARIABLE(ENABLE_MSTK_Mesh)             
#cmake_dependent_option(ENABLE_MSTK_Mesh "Build Amanzi with the MSTK mesh framework" OFF
#                       "ENABLE_METIS" ON )
#set_feature_info(MSTK_Mesh
#                 ENABLE_MSTK_Mesh
#                 "A mesh framework"
#                 )
#PRINT_VARIABLE(ENABLE_MSTK_Mesh)             
#if (ENABLE_MSTK_Mesh)
#    find_package(MSTK REQUIRED)
#endif() 

##############################################################################
# METIS - http://glaros.dtc.umn.edu/gkhome/metis/metis/download
##############################################################################
option(ENABLE_METIS "Mesh partitioning library" OFF)
set_feature_info(METIS
                 ENABLE_METIS
                 "Mesh partitioning library"
                 )
if (ENABLE_METIS)
    find_package(METIS REQUIRED)
endif() 

##############################################################################
# CGNS - http://www.cgns.sourceforge.net/
##############################################################################
option(ENABLE_CGNS  "Build Amanzi output library with CGNS" OFF)
set_feature_info(CGNS
                  ENABLE_CGNS
                 "CFD General Notation System"
                 "http://cgns.sourceforge.net"
                 "Required to produce VisIt files"
                 )
if (ENABLE_CGNS)
    find_package(CGNS REQUIRED)
endif() 

##############################################################################
# UnitTest++ - http://unittest-cpp.sourceforge.net/
##############################################################################
option(ENABLE_UnitTest "Build Amanzi unit tests. Requires UnitTest++" ON)
set_feature_info(UnitTest
                 ENABLE_UnitTest
                 "C++ unit test framework"
                 )
if (ENABLE_UnitTest)
    find_package(UnitTest)
endif()    








