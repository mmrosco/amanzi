/*
  Multiphase PK

  Copyright 2010-2013 held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include "GraphFE.hh"
#include "MatrixFE.hh"
#include "OperatorUtils.hh"
#include "Schema.hh"
#include "SuperMap.hh"

#include "FlattenedTreeOperator.hh"

namespace Amanzi {
namespace Operators {

FlattenedTreeOperator::FlattenedTreeOperator(Teuchos::RCP<const TreeVectorSpace> tvs) 
{
  tvs_ = tvs;
  block_diagonal_ = false;

  Schema schema;
  tvs_flat_ = Teuchos::rcp(new TreeVectorSpace());

  int n_blocks(0);
  bool flag(false);

  for (auto it = tvs->begin(); it != tvs->end(); ++it) {
    auto cvs = (*it)->Data();
    int nvec = cvs->NumVectors(*cvs->begin());

    auto cvs0 = Teuchos::rcp(new CompositeVectorSpace());
    cvs0->SetMesh(cvs->Mesh())->SetGhosted(true);
    cvs0->SetOwned(false);

    for (auto kt = cvs->begin(); kt != cvs->end(); ++kt) {
      AMANZI_ASSERT(nvec == cvs->NumVectors(*kt));
      cvs0->AddComponent(*kt, schema.StringToKind(*kt), 1);  // FIXME
    }
    CompositeVector cv(*cvs0);

    for (int n = 0; n < nvec; ++n) {
      Teuchos::RCP<TreeVectorSpace> tvs0 = Teuchos::rcp(new TreeVectorSpace());
      tvs0->SetData(cvs0);
      tvs_flat_->PushBack(tvs0);
    }
    n_blocks += nvec;
    if (nvec > 1) flag = true;
  }

  // resize the blocks
  blocks_.resize(n_blocks, Teuchos::Array<Teuchos::RCP<const Operator> >(n_blocks, Teuchos::null));

  // first map supports solvers which use casual tree vectors
  // second map helps matrix assembly from smaller-size blocks
  smap_ = createSuperMap(*tvs_);
  if (flag) 
    smap_flat_ = createSuperMap(*tvs_flat_);
  else
    smap_flat_ = smap_;
}


/* ******************************************************************
* Symbolic assemble global matrix from matrices of block operators. 
****************************************************************** */
void FlattenedTreeOperator::SymbolicAssembleMatrix()
{
  int n_blocks = blocks_.size();

  int schema = 0;
  std::vector<CompositeVectorSpace> cvs_vec;
  std::vector<std::string> cvs_names;

  // Check that each row has at least one non-null operator block
  // and save the position of this block, preferably diagonal.
  Teuchos::RCP<const Operator> an_op;
  for (int row = 0; row != n_blocks; ++row) {
    int block_col(-1);
    for (int col = 0; col != n_blocks; ++col) {
      if (blocks_[row][col] != Teuchos::null) {
        an_op = blocks_[row][col];
        if (block_col != row) block_col = col;
      }
    }
    AMANZI_ASSERT(block_col >= 0);

    cvs_vec.push_back(blocks_[row][block_col]->RangeMap());
    cvs_names.push_back(std::to_string(row));
  }

  // NOTE: this probably needs to be fixed for differing meshes. -etc
  int row_size = MaxRowSize(*an_op->DomainMap().Mesh(), schema, n_blocks);
  auto graph = Teuchos::rcp(new GraphFE(smap_flat_->Map(), 
      smap_flat_->GhostedMap(), smap_flat_->GhostedMap(), row_size));

  // fill the graph
  for (int row = 0; row != n_blocks; ++row) {
    for (int col = 0; col != n_blocks; ++col) {
      Teuchos::RCP<const Operator> block = blocks_[row][col];
      if (block != Teuchos::null) {
        block->SymbolicAssembleMatrix(*smap_flat_, *graph, row, col);
      }
    }
  }

  // assemble the graph
  int ierr = graph->FillComplete(smap_flat_->Map(), smap_flat_->Map());
  AMANZI_ASSERT(!ierr);

  // create the matrix
  Amat_ = Teuchos::rcp(new MatrixFE(graph));
  A_ = Amat_->Matrix();
}


/* ******************************************************************
* Assemble global matrix from elemental matrices of block operators.
****************************************************************** */
void FlattenedTreeOperator::AssembleMatrix()
{
  int n_blocks = blocks_.size();
  Amat_->Zero();

  // check that each row has at least one non-null operator block
  for (int row = 0; row != n_blocks; ++row) {
    for (int col = 0; col != n_blocks; ++col) {
      Teuchos::RCP<const Operator> block = blocks_[row][col];
      if (block != Teuchos::null) {
        block->AssembleMatrix(*smap_flat_, *Amat_, row, col);
      }
    }
  }

  int ierr = Amat_->FillComplete();
  AMANZI_ASSERT(!ierr);
}


/* ******************************************************************
* Calculate Y = A * X using matrix-free matvec on blocks of operators.
****************************************************************** */
int FlattenedTreeOperator::Apply(const TreeVector& X, TreeVector& Y) const {
  return ApplyAssembled(X, Y);
}

}  // namespace Operators
}  // namespace Amanzi
