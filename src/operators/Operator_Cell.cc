/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
           Ethan Coon (ecoon@lanl.gov)

  Operator whose unknowns are CELLs.
*/

#include "DenseMatrix.hh"
#include "Op_Cell_Cell.hh"
#include "Op_Face_Cell.hh"

#include "SuperMap.hh"
#include "GraphFE.hh"
#include "MatrixFE.hh"
#include "Operator_Cell.hh"

namespace Amanzi {
namespace Operators {

/* ******************************************************************
* Apply a source which may or may not have cell volume included already. 
****************************************************************** */
void Operator_Cell::UpdateRHS(const CompositeVector& source,
                              bool volume_included)
{
  if (volume_included) {
    Operator::UpdateRHS(source);
  } else {
    // not yet implemented --etc
    AMANZI_ASSERT(false);
    // Epetra_MultiVector& rhs_c = *rhs_->ViewComponent("cell", false);
    // const Epetra_MultiVector& source_c = *source.ViewComponent("cell", false);
    // for (int c = 0; c != ncells_owned; ++c) {
    //   rhs_c[0][c] += source_c[0][c] * mesh_->cell_volume(c);
    // }
  }
}


/* ******************************************************************
* Visit methods for Apply.
* Apply the local matrices directly as schema is a subset of 
* assembled schema.
****************************************************************** */
int Operator_Cell::ApplyMatrixFreeOp(const Op_Cell_Cell& op,
                                     const CompositeVector& X, CompositeVector& Y) const
{
  auto Xc = X.ViewComponent<AmanziDefaultDevice>("cell");
  auto Yc = Y.ViewComponent<AmanziDefaultDevice>("cell");
  Kokkos::parallel_for(
      "Operator_Cell::ApplyMatrixFreeOp Cell_Cell",
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>{ {0,0}, {Yc.extent(0), Yc.extent(1)} },
      KOKKOS_LAMBDA(const int c, const int k) {
        Yc(c,k) += Xc(c,k) * op.data(c,k);
      });
  return 0;
}


/* ******************************************************************
* Apply the local matrices directly as schema is a subset of
* assembled schema
****************************************************************** */
int Operator_Cell::ApplyMatrixFreeOp(const Op_Face_Cell& op,
                                     const CompositeVector& X, CompositeVector& Y) const
{
  // not yet implemented --etc
  AMANZI_ASSERT(false);
  AMANZI_ASSERT(op.data.extent(0) == nfaces_owned);
  
  X.ScatterMasterToGhosted();
  Y.putScalarGhosted(0.);

  {
    auto Xc = X.ViewComponent<AmanziDefaultDevice>("cell", true);
    auto Yc = Y.ViewComponent<AmanziDefaultDevice>("cell", true);

    typedef Kokkos::TeamPolicy<> TeamPolicy_type ;
    typedef typename TeamPolicy_type::member_type Team_type ;

    Kokkos::parallel_for( // loop over local matrices
      "Operator_Cell::ApplyMatrixFreeOp Face_Cell",
      TeamPolicy_type(op.data.extent(0), Kokkos::AUTO()),
      KOKKOS_LAMBDA(Team_type team) {
        const int f = team.team_rank();
        AmanziMesh::Entity_ID_View cells;
        mesh_->face_get_cells(f, AmanziMesh::Parallel_type::ALL, cells);
        Kokkos::parallel_for( // loop over each row of the local matrix
            //            "Operator_Cell::ApplyMatrixFreeOp Face_Cell MatMult Rows",
            Kokkos::TeamThreadRange(team, cells.extent(0)),
            [&] (const int& i) {
              double sum;
              Kokkos::parallel_reduce( // reduce over each column of the row
                  //    "Operator_Cell::ApplyMatrixFreeOp Face_Cell MatMult Cols",
                  Kokkos::ThreadVectorRange(team, cells.extent(0)),
                  [=] (const int& j, double& psum) {
                    psum += op.data(f,j+i*cells.extent(0)) * Xc(cells(j),0);
                  }, sum);
              // atomic add required as multiple local matrices may contribute
              // to the same cell in Y.
              Kokkos::atomic_add(&Yc(cells(i),0), sum);
            });
      });              

  // OLD SIMPLE CODE for posterity!  Julien, help!
  // for (int f = 0; f != nfaces_owned; ++f) {
  //   mesh_->face_get_cells(f, AmanziMesh::Parallel_type::ALL, &cells);
  //   int ncells = cells.size();
  //   WhetStone::DenseVector v(ncells), av(ncells);
  //   for (int n = 0; n != ncells; ++n) {
  //     v(n) = Xc[0][cells[n]];
  //   }

  //   const WhetStone::DenseMatrix& Aface = op.matrices[f];
  //   Aface.Multiply(v, av, false);

  //   for (int n = 0; n != ncells; ++n) {
  //     Yc[0][cells[n]] += av(n);
  //   }
  // }
  }
  Y.GatherGhostedToMaster("cell");
  return 0;
}


// /* ******************************************************************
// * Visit methods for symbolic assemble.
// * Insert cell-based diagonal matrix.
// ****************************************************************** */
// void Operator_Cell::SymbolicAssembleMatrixOp(const Op_Cell_Cell& op,
//                                              const SuperMap& map, GraphFE& graph,
//                                              int my_block_row, int my_block_col) const
// {
//   const std::vector<int>& cell_row_inds = map.GhostIndices(my_block_row, "cell", 0);
//   const std::vector<int>& cell_col_inds = map.GhostIndices(my_block_col, "cell", 0);

//   int ierr(0);
//   for (int c = 0; c != ncells_owned; ++c) {
//     int row = cell_row_inds[c];
//     int col = cell_col_inds[c];

//     ierr |= graph.InsertMyIndices(row, 1, &col);
//   }
//   AMANZI_ASSERT(!ierr);
// }


// /* ******************************************************************
// * Insert each face neighboring cells (ELEMENT/BASE=face, DOFs=cell)
// ****************************************************************** */
// void Operator_Cell::SymbolicAssembleMatrixOp(const Op_Face_Cell& op,
//                                              const SuperMap& map, GraphFE& graph,
//                                              int my_block_row, int my_block_col) const
// {
//   AMANZI_ASSERT(op.matrices.size() == nfaces_owned);

//   std::vector<int> lid_r;
//   std::vector<int> lid_c;

//   const std::vector<int>& cell_row_inds = map.GhostIndices(my_block_row, "cell", 0);
//   const std::vector<int>& cell_col_inds = map.GhostIndices(my_block_col, "cell", 0);

//   int ierr(0);
//   AmanziMesh::Entity_ID_List cells; 
//   for (int f = 0; f != nfaces_owned; ++f) {
//     mesh_->face_get_cells(f, AmanziMesh::Parallel_type::ALL, &cells);
    
//     int ncells = cells.size();
//     lid_r.resize(ncells);
//     lid_c.resize(ncells);    
//     for (int n = 0; n != ncells; ++n) {
//       lid_r[n] = cell_row_inds[cells[n]];
//       lid_c[n] = cell_col_inds[cells[n]];
//     }

//     ierr |= graph.InsertMyIndices(ncells, lid_r.data(), ncells, lid_c.data());
//   }

//   AMANZI_ASSERT(!ierr);
// }


// /* ******************************************************************
// * Visit methods for assemble
// * Insert each cells neighboring cells.
// ****************************************************************** */
// void Operator_Cell::AssembleMatrixOp(const Op_Cell_Cell& op,
//                                      const SuperMap& map, MatrixFE& mat,
//                                      int my_block_row, int my_block_col) const
// {
//   AMANZI_ASSERT(op.diag->NumVectors() == 1);

//   const std::vector<int>& cell_row_inds = map.GhostIndices(my_block_row, "cell", 0);
//   const std::vector<int>& cell_col_inds = map.GhostIndices(my_block_col, "cell", 0);

//   int ierr(0);
//   for (int c = 0; c != ncells_owned; ++c) {
//     int row = cell_row_inds[c];
//     int col = cell_col_inds[c];

//     ierr |= mat.SumIntoMyValues(row, 1, &(*op.diag)[0][c], &col);
//   }
//   AMANZI_ASSERT(!ierr);
// }


// void Operator_Cell::AssembleMatrixOp(const Op_Face_Cell& op,
//                                      const SuperMap& map, MatrixFE& mat,
//                                      int my_block_row, int my_block_col) const
// {
//   AMANZI_ASSERT(op.matrices.size() == nfaces_owned);
  
//   // ELEMENT: face, DOF: cell
//   std::vector<int> lid_r;
//   std::vector<int> lid_c;

//   const std::vector<int>& cell_row_inds = map.GhostIndices(my_block_row, "cell", 0);
//   const std::vector<int>& cell_col_inds = map.GhostIndices(my_block_col, "cell", 0);

//   int ierr(0);
//   AmanziMesh::Entity_ID_List cells;
//   for (int f = 0; f != nfaces_owned; ++f) {
//     mesh_->face_get_cells(f, AmanziMesh::Parallel_type::ALL, &cells);
    
//     int ncells = cells.size();
//     lid_r.resize(ncells);
//     lid_c.resize(ncells);    
//     for (int n = 0; n != ncells; ++n) {      
//       lid_r[n] = cell_row_inds[cells[n]];
//       lid_c[n] = cell_col_inds[cells[n]];
//     }
   
//     ierr |= mat.SumIntoMyValues(lid_r.data(), lid_c.data(), op.matrices[f]);
//     AMANZI_ASSERT(ierr>=0);
//   }
//   AMANZI_ASSERT(ierr>=0);
// }

}  // namespace Operators
}  // namespace Amanzi

