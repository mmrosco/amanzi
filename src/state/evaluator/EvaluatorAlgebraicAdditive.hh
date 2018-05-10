/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/*
  Copyright 2010-201x held jointly, see COPYRIGHT.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon
*/

//! EvaluatorAlgebraicAdditive adds its dependencies.

/*!

  
*/

#ifndef STATE_EVALUATOR_ALGEBRAIC_ADDITIVE_HH_
#define STATE_EVALUATOR_ALGEBRAIC_ADDITIVE_HH_

#include <string>
#include <vector>

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"

#include "exceptions.hh"

#include "EvaluatorAlgebraic.hh"

namespace Amanzi {

// By default, this class adds nothing on top of EvaluatorSecondary.
// Specializations can do useful things though.
template <typename Data_t, typename DataFactory_t = NullFactory>
class EvaluatorAlgebraicAdditive : public EvaluatorAlgebraic<Data_t, DataFactory_t> {
public:
  EvaluatorAlgebraicAdditive(Teuchos::ParameterList &plist) :
      EvaluatorAlgebraic<Data_t,DataFactory_t>(plist)
  {
    for (const auto& dep : this->dependencies_) {
      Key pname = dep.first + " coefficient";
      Key pname_full = dep.first + ":" + dep.second + " coefficient";
      if (plist.isParameter(pname_full)) coefs_[pname_full] = plist.get<double>(pname_full);
      else if (plist.isParameter(pname)) coefs_[pname_full] = plist.get<double>(pname);
      else coefs_[pname_full] = 1.0;
    }
  }
    

  EvaluatorAlgebraicAdditive(const EvaluatorAlgebraicAdditive &other) = default;
  virtual Teuchos::RCP<Evaluator> Clone() const override {
    return Teuchos::rcp(new EvaluatorAlgebraicAdditive(*this));
  }

protected:

  virtual void Evaluate_(const State &S, Data_t &result) override {
    result.PutScalar(0.);
    for (const auto& dep : this->dependencies_) {
      const auto& term = S.Get<Data_t>(dep.first, dep.second);
      double coef = coefs_[dep.first+":"+dep.second];
      result.Update(coef, term, 1.0);
    }
  }
  
  virtual void EvaluatePartialDerivative_(const State &S,
          const Key &wrt_key, const Key &wrt_tag, Data_t &result) override {
    Key pname_full = wrt_key+":"+wrt_tag;
    result.PutScalar(coefs_[pname_full]);    
  }

 protected:
  std::map<Key,double> coefs_;

 private:
  static Utils::RegisteredFactory<Evaluator, EvaluatorAlgebraicAdditive<Data_t,DataFactory_t>> fac_;
  
  
};


} // namespace Amanzi

#endif
