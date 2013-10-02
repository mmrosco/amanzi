/*
This is the flow component of the Amanzi code. 

Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided in the top-level COPYRIGHT file.

Authors: Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#ifndef AMANZI_TI_SPECS_HH_
#define AMANZI_TI_SPECS_HH_

#include "FlowDefs.hh"


namespace Amanzi {
namespace AmanziFlow {

// We cannot initialize linear solvers with a parameter list
class LinearSolver_Specs {
 public:
  LinearSolver_Specs() {
    num_itrs = 0;
    max_itrs = 911;
    convergence_tol = 1.911e-16;
    preconditioner_name = "undefined";
    preconditioner_method = FLOW_PRECONDITIONER_HYPRE_AMG;  // Must be equal to ST_PRECOND
  }
  ~LinearSolver_Specs() {};

 public:
  int num_itrs, max_itrs;
  std::string solver_name;
  std::string preconditioner_name;
  int preconditioner_method;
  double convergence_tol; 
};

class TI_Specs {
 public:
  TI_Specs() { 
    ti_method = FLOW_TIME_INTEGRATION_BDF1;
    ti_method_name = "none";
    preconditioner_name = "undefined";
    preconditioner_method = FLOW_PRECONDITIONER_HYPRE_AMG;  // Must equal to ST_PRECOND
    num_itrs = max_itrs = 0;
    error_control_options = 0;
    dT_method = 0;
    T0 = T1 = dT0 = dTmax = 0.0;
    dTfactor = 1.0;
    atol = rtol = 1e-3;
    residual_tol = 0.0;
    initialize_with_darcy = false;
    clip_saturation = -1.0;
    clip_pressure = -1e+10;
    pressure_lambda_constraints = true;

    ls_specs.solver_name = "AztecOO";
    ls_specs_ini.solver_name = "PCG with Hypre AMG";
    ls_specs_constraints.solver_name = "AztecOO";
  }
  ~TI_Specs() {};

 public:
  int ti_method;
  std::string ti_method_name;

  LinearSolver_Specs ls_specs;
  std::string preconditioner_name;
  int preconditioner_method;
  int num_itrs, max_itrs;

  int dT_method, error_control_options;
  double T0, T1, dT0, dTmax, dTfactor;
  double atol, rtol, residual_tol; 

  LinearSolver_Specs ls_specs_ini;  // initialization options
  bool initialize_with_darcy;
  double clip_saturation, clip_pressure;

  bool pressure_lambda_constraints;
  LinearSolver_Specs ls_specs_constraints;

  std::vector<std::pair<double, double> > dT_history;  // statistics (relocate to debug?)
};

}  // namespace AmanziFlow
}  // namespace Amanzi

#endif

