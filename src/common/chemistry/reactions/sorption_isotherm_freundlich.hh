/*
  Chemistry 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Class for Freundlich isotherm
*/

#ifndef AMANZI_CHEMISTRY_SORPTION_ISOTHERM_FREUNDLICH_HH_
#define AMANZI_CHEMISTRY_SORPTION_ISOTHERM_FREUNDLICH_HH_

#include <vector>

#include "sorption_isotherm.hh"

namespace Amanzi {
namespace AmanziChemistry {

class SorptionIsothermFreundlich : public SorptionIsotherm {
 public:
  SorptionIsothermFreundlich();
  SorptionIsothermFreundlich(const double KD, const double n);
  ~SorptionIsothermFreundlich() {};

  // returns sorbed concentration
  double Evaluate(const Species& primarySpecies);
  double EvaluateDerivative(const Species& primarySpecies);
  void Display(void) const;

  double KD(void) const { return KD_; }
  void set_KD(const double KD) { KD_ = KD; }
  double n(void) const { return n_; }
  void set_n(const double n) { n_ = n; }

  const std::vector<double>& GetParameters(void);
  void SetParameters(const std::vector<double>& params);

 private:
  double KD_; // distribution coefficient
  double n_; // chemical-specific constant
  std::vector<double> params_;
};

}  // namespace AmanziChemistry
}  // namespace Amanzi
#endif
