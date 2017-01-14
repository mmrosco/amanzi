/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
           Ethan Coon (ecoon@lanl.gov)

  Upwind-based advection operator for a scalr field.
*/

#ifndef AMANZI_OPERATOR_ADVECTION_UPWIND_HH_
#define AMANZI_OPERATOR_ADVECTION_UPWIND_HH_

#include "Epetra_IntVector.h"

#include "Advection.hh"

namespace Amanzi {
namespace Operators {

class AdvectionUpwind : public Advection {
 public:
  AdvectionUpwind(Teuchos::ParameterList& plist,
                  Teuchos::RCP<Operator> global_op) :
      Advection(plist, global_op)
  {
    InitAdvection_(plist);
  }

  AdvectionUpwind(Teuchos::ParameterList& plist,
                  Teuchos::RCP<const AmanziMesh::Mesh> mesh) :
      Advection(plist, mesh)
  {
    InitAdvection_(plist);
  }

  // required members 
  // -- setup
  virtual void Setup(const CompositeVector& u);
  // -- data
  virtual void UpdateMatrices(const CompositeVector& u);
  virtual void UpdateMatrices(const CompositeVector& u, const CompositeVector& dhdT);
  // -- boundary conditions
  virtual void ApplyBCs(const Teuchos::RCP<BCs>& bc, bool primary);

  // -- results -- determine advected flux of u
  void UpdateFlux(const CompositeVector& h , const CompositeVector& u,
                  const Teuchos::RCP<BCs>& bc, CompositeVector& flux);
  
 private:
  void InitAdvection_(Teuchos::ParameterList& plist);
  void IdentifyUpwindCells_(const CompositeVector& u);

 private:
  Teuchos::RCP<Epetra_IntVector> upwind_cell_, downwind_cell_;
};

}  // namespace Operators
}  // namespace Amanzi

#endif

