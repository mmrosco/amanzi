/*
  This is the operators component of the Amanzi code. 

  Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_OPERATORS_UTILS_HH_
#define AMANZI_OPERATORS_UTILS_HH_

#include "Teuchos_RCP.hpp"

class Epetra_Vector;

namespace Amanzi {

class CompositeVector;

namespace Operators {

class SuperMap;

// Nonmember composite vector to/from super vector
int
CopyCompositeVectorToSuperVector(const SuperMap& map, const CompositeVector& cv,
        Epetra_Vector& sv, int dofnum=0);

int
CopyCompositeVectorToSuperVector(const SuperMap& map, const CompositeVector& cv,
        Teuchos::RCP<Epetra_Vector>& sv, int dofnum=0);

int
CopySuperVectorToCompositeVector(const SuperMap& map,const Epetra_Vector& sv,
        CompositeVector& cv, int dofnum=0);

} // namespace Operators
} // namespace Amanzi


#endif