/*
  This is the energy component of the ATS and Amanzi codes. 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon

  Linear internal energy model -- function of Cv and temperature
  UNITS: J/{mol/kg}
*/

#ifndef AMANZI_ENERGY_IEM_LINEAR_HH_
#define AMANZI_ENERGY_IEM_LINEAR_HH_

#include "Teuchos_ParameterList.hpp"

#include "iem.hh"
#include "factory.hh"

namespace Amanzi {
namespace Energy {

class IEMLinear : public IEM {
 public:
  explicit IEMLinear(Teuchos::ParameterList& plist);

  bool IsMolarBasis() { return molar_basis_; }

  double InternalEnergy(double temp);
  double DInternalEnergyDT(double temp) { return Cv_; }

 private:
  virtual void InitializeFromPlist_();

  Teuchos::ParameterList plist_;

  double Cv_; // units: J/({mol/kg}-K)
  double T_ref_; // units: K
  bool molar_basis_;

 private:
  // iem factor registration
  static Utils::RegisteredFactory<IEM,IEMLinear> factory_;
};

}  // namespace Energy
}  // namespace Amanzi

#endif