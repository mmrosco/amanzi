# -*- mode: cmake -*-

#
#  Amanzi
#    Equations of state
#

#
# Define a project name
# After this command the following varaibles are defined
#   EOS_SOURCE_DIR
#   EOS_BINARY_DIR
# Other projects (subdirectories) can reference this directory
# through these variables.
project(EOS)

# Amanzi include directories
include_directories(${ATK_SOURCE_DIR})

#
# Library: eos
#
set(eos_src_files 
    EffectivePressureEvaluator.cc
    EOSEvaluator.cc
    EOSFactory.cc 
    EOS_Constant.cc
    EOS_IdealGas.cc
    EOS_VaporInGas.cc
    EOS_Water.cc
    IsobaricEOSEvaluator.cc
    MolarFractionGasEvaluator.cc
    ThermalConductivity_Water.cc
    VaporPressure_Water.cc
    VaporPressureBaseFactory.cc
    Viscosity_Water.cc
    Viscosity_Constant.cc
    ViscosityBaseFactory.cc
    ViscosityEvaluator.cc
   )

add_amanzi_library(eos
                   SOURCE ${eos_src_files} HEADERS ${eos_inc_files}
		   LINK_LIBS data_structures state atk ${Epetra_LIBRARIES})
if (APPLE AND BUILD_SHARED_LIBS)
  set_target_properties(eos PROPERTIES LINK_FLAGS "-Wl,-undefined,dynamic_lookup")
endif()			  

#
# Install Targets
#
set(eos_inc_files 
    EOS.hh
    EOSEvaluator.hh
    EOSFactory.hh
    EOS_Constant.hh
    EOS_ConstantMolarMass.hh
    EOS_IdealGas.hh
    EOS_VaporInGas.hh
    EOS_Water.hh
    eos_registration.hh
    IsobaricEOSEvaluator.hh
    MolarFractionGasEvaluator.hh
    ThermalConductivity_Water.hh
    VaporPressure_Base.hh
    VaporPressure_Water.hh
    VaporPressureBaseFactory.hh
    Viscosity_Base.hh
    Viscosity_Constant.hh
    Viscosity_Water.hh
    ViscosityBaseFactory.hh
    ViscosityEvaluator.hh)

# install headers into a subdirectory
install(FILES ${eos_inc_files} DESTINATION include/amanzi)

if (BUILD_TESTS) 
endif()

