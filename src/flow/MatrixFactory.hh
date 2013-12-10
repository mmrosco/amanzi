/*
  This is the flow component of the Amanzi code. 

  Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
           Daniil Svyatskiy
*/

#include <strings.h>

#include "Epetra_Map.h"
#include "Epetra_MultiVector.h"
#include "Teuchos_RCP.hpp"

#include "CompositeVector.hh"
#include "CompositeVectorSpace.hh"

#include "Mesh.hh"
#include "State.hh"
#include "RelativePermeability.hh"
#include "Matrix.hh"
#include "Matrix_MFD.hh"
#include "Matrix_TPFA.hh"

namespace Amanzi {
namespace AmanziFlow {

class MatrixFactory {
 public:
  MatrixFactory() {};
  ~MatrixFactory() {};

  Teuchos::RCP<FlowMatrix> Create(Teuchos::RCP<State> S,
                                  Teuchos::RCP<RelativePermeability> rel_perm, 
                                  Teuchos::ParameterList& plist)
  {
    std::string name = plist.get<std::string>("matrix", "mfd");
    if (name == "mfd") {
      Teuchos::RCP<Matrix_MFD> matrix = Teuchos::rcp(new Matrix_MFD(S, rel_perm));
      matrix->Init();

      return matrix;
    } else if (name == "tpfa") {
      Teuchos::RCP<Matrix_TPFA> matrix = Teuchos::rcp(new Matrix_TPFA(S, rel_perm));
      matrix->Init();

      return matrix;
    }
  };
};

}  // namespace AmanziFlow
}  // namespace Amanzi

