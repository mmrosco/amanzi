/*
  Flow PK

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// TPLs
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"
#include "UnitTest++.h"

// Amanzi
#include "GMVMesh.hh"
#include "MeshFactory.hh"
#include "State.hh"

// Flow
#include "Darcy_PK.hh"

/* **************************************************************** */
TEST(FLOW_2D_TRANSIENT_DARCY) {
  using namespace Teuchos;
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::AmanziGeometry;
  using namespace Amanzi::Flow;

  Epetra_MpiComm comm(MPI_COMM_WORLD);
  int MyPID = comm.MyPID();

  if (MyPID == 0) std::cout << "Test: 2D transient Darcy, polygonal mesh" << std::endl;

  /* read parameter list */
  std::string xmlFileName = "test/flow_darcy_dual2D.xml";
  Teuchos::RCP<Teuchos::ParameterList> plist = Teuchos::getParametersFromXmlFile(xmlFileName);

  /* create a MSTK mesh framework */
  ParameterList region_list = plist->get<Teuchos::ParameterList>("Regions");
  GeometricModelPtr gm = new GeometricModel(2, region_list, &comm);

  FrameworkPreference pref;
  pref.clear();
  pref.push_back(MSTK);
  pref.push_back(STKMESH);

  MeshFactory meshfactory(&comm);
  meshfactory.preference(pref);
  RCP<const Mesh> mesh = meshfactory("test/dual2D.exo", gm);

  // create a state and populate it
  Teuchos::ParameterList state_list = plist->sublist("State");
  RCP<State> S = rcp(new State(state_list));
  S->RegisterDomainMesh(rcp_const_cast<Mesh>(mesh));

  Teuchos::RCP<Darcy_PK> DPK = Teuchos::rcp(new Darcy_PK(plist, "Flow", S));
  DPK->Setup(S.ptr());
  S->Setup();
  S->InitializeFields();

  // modify the default state for the problem at hand 
  // -- permeability
  std::string passwd("flow"); 
  Epetra_MultiVector& K = *S->GetFieldData("permeability", passwd)->ViewComponent("cell", false);
  for (int c = 0; c < K.MyLength(); c++) {
    K[0][c] = 0.1;
    K[1][c] = 2.0;
  }
  S->GetField("permeability", "flow")->set_initialized();

  // -- fluid density and viscosity
  *S->GetScalarData("fluid_density", passwd) = 1.0;
  S->GetField("fluid_density", "flow")->set_initialized();

  *S->GetScalarData("fluid_viscosity", passwd) = 1.0;
  S->GetField("fluid_viscosity", "flow")->set_initialized();

  // -- gravity
  Epetra_Vector& gravity = *S->GetConstantVectorData("gravity", "state");
  gravity[1] = -1.0;
  S->GetField("gravity", "state")->set_initialized();

  // create the initial pressure function
  Epetra_MultiVector& p = *S->GetFieldData("pressure", passwd)->ViewComponent("cell", false);

  for (int c = 0; c < p.MyLength(); c++) {
    const Point& xc = mesh->cell_centroid(c);
    p[0][c] = xc[1] * (xc[1] + 2.0);
  }
  S->GetField("pressure", "flow")->set_initialized();

  // initialize Darcy process kernel.
  DPK->Initialize(S.ptr());

  // transient solution
  double t_old(0.0), t_new, dt(0.1);
  for (int n = 0; n < 2; n++) {
    t_new = t_old + dt;

    DPK->AdvanceStep(t_old, t_new);
    DPK->CommitStep(t_old, t_new, S);

    t_old = t_new;

    if (MyPID == 0) {
      GMV::open_data_file(*mesh, (std::string)"flow.gmv");
      GMV::start_data();
      GMV::write_cell_data(p, 0, "pressure");
      GMV::close_data_file();
    }
  }
}
