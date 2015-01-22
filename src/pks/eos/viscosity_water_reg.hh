/*
  This is the EOS component of the ATS and Amanzi codes.
   
  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)

  EOS for liquid water.  See the permafrost physical properties notes for
  references and documentation of this EOS at:
*/

#include "errors.hh"
#include "viscosity_water.hh"

namespace Amanzi {
namespace Relations {

// registry of method
Utils::RegisteredFactory<ViscosityRelation,ViscosityWater> ViscosityWater::factory_("liquid water");

}  // namespace Relations
}  // namespace Amanzi