/*
  This is the transport component of the Amanzi code. 

  Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <set>
#include <string>
#include <vector>

#include "Teuchos_RCP.hpp"

#include "errors.hh"
#include "MultiFunction.hh"
#include "GMVMesh.hh"

#include "Mesh.hh"
#include "Transport_PK.hh"
#include "TransportBCFactory.hh"

namespace Amanzi {
namespace Transport {

/* ******************************************************************
* Routine processes parameter list. It needs to be called only once
* on each processor.                                                     
****************************************************************** */
void Transport_PK::ProcessParameterList()
{
  Teuchos::ParameterList transport_list;
  transport_list = parameter_list;

  // create verbosity object
  vo_ = new VerboseObject("TransportPK", transport_list); 

  // global transport parameters
  cfl_ = transport_list.get<double>("cfl", 1.0);

  spatial_disc_order = transport_list.get<int>("spatial discretization order", 1);
  if (spatial_disc_order < 1 || spatial_disc_order > 2) spatial_disc_order = 1;
  temporal_disc_order = transport_list.get<int>("temporal discretization order", 1);
  if (temporal_disc_order < 1 || temporal_disc_order > 2) temporal_disc_order = 1;

  std::string advection_limiter_name = transport_list.get<std::string>("advection limiter");
  ProcessStringAdvectionLimiter(advection_limiter_name, &advection_limiter);

  num_aqueous = transport_list.get<int>("number of aqueous components", component_names_.size());
  num_gaseous = transport_list.get<int>("number of gaseous components", 0);

  // transport dispersion (default is none)
  dispersion_solver = transport_list.get<std::string>("solver", "missing");

  if (transport_list.isSublist("material properties")) {
    Teuchos::ParameterList& dlist = transport_list.sublist("material properties");

    if (solvers_list.isSublist(dispersion_solver)) {
      Teuchos::ParameterList& slist = solvers_list.sublist(dispersion_solver);
      dispersion_preconditioner = slist.get<std::string>("preconditioner", "identity");
    } else {
      Errors::Message msg;
      msg << "Transport PK: dispersivity solver does not exist.\n";
      Exceptions::amanzi_throw(msg);  
    }

    std::string method_name = dlist.get<std::string>("numerical method", "none");
    // ProcessStringDispersionMethod(method_name, &dispersion_method);

    int nblocks = 0; 
    for (Teuchos::ParameterList::ConstIterator i = dlist.begin(); i != dlist.end(); i++) {
      if (dlist.isSublist(dlist.name(i))) nblocks++;
    }

    material_properties_.resize(nblocks);
    dispersion_models_ = TRANSPORT_DISPERSIVITY_MODEL_NULL;

    int iblock = 0, iblock0 = 0;
    for (Teuchos::ParameterList::ConstIterator i = dlist.begin(); i != dlist.end(); i++) {
      if (dlist.isSublist(dlist.name(i))) {
        material_properties_[iblock] = Teuchos::rcp(new MaterialProperties());

        Teuchos::ParameterList& model_list = dlist.sublist(dlist.name(i));

        std::string model_name = model_list.get<std::string>("model", "none");
        ProcessStringDispersionModel(model_name, &(material_properties_[iblock]->model));
        dispersion_models_ |= material_properties_[iblock]->model;

        material_properties_[iblock]->alphaL = model_list.get<double>("alphaL", 0.0);
        material_properties_[iblock]->alphaT = model_list.get<double>("alphaT", 0.0);
        material_properties_[iblock]->tau[0] = model_list.get<double>("aqueous tortuosity", 0.0);
        material_properties_[iblock]->tau[1] = model_list.get<double>("gaseous tortuosity", 0.0);
        material_properties_[iblock]->regions = model_list.get<Teuchos::Array<std::string> >("regions").toVector();

        // run-time verification
        if (material_properties_[iblock]->alphaL == 0.0 && 
            material_properties_[iblock]->alphaT == 0.0) {
          if (vo_->getVerbLevel() >= Teuchos::VERB_LOW) {
            Teuchos::OSTab tab = vo_->getOSTab();
            *vo_->os() << vo_->color("yellow") << "Zero dispersion for sublist \"" 
                       << dlist.name(i) << "\"" << vo_->reset() << std::endl;
          }
          iblock0++;
        }
        iblock++;
      }
    }
    if (iblock0 == iblock) dispersion_models_ = TRANSPORT_DISPERSIVITY_MODEL_NULL;
  }

  // transport diffusion (default is none)
  diffusion_phase_.resize(TRANSPORT_NUMBER_PHASES, Teuchos::null);

  if (transport_list.isSublist("molecular diffusion")) {
    Teuchos::ParameterList& dlist = transport_list.sublist("molecular diffusion");
    if (dlist.isParameter("aqueous names")) { 
      diffusion_phase_[0] = Teuchos::rcp(new DiffusionPhase());
      diffusion_phase_[0]->names() = dlist.get<Teuchos::Array<std::string> >("aqueous names").toVector();
      diffusion_phase_[0]->values() = dlist.get<Teuchos::Array<double> >("aqueous values").toVector();
    }

    if (dlist.isParameter("gas names")) { 
      diffusion_phase_[1] = Teuchos::rcp(new DiffusionPhase());
      diffusion_phase_[1]->names() = dlist.get<Teuchos::Array<std::string> >("gas names").toVector();
      diffusion_phase_[1]->values() = dlist.get<Teuchos::Array<double> >("gas values").toVector();
    }
  }

  // control parameter
  internal_tests = transport_list.get<std::string>("enable internal tests", "no") == "yes";
  tests_tolerance = transport_list.get<double>("internal tests tolerance", TRANSPORT_CONCENTRATION_OVERSHOOT);
  dT_debug = transport_list.get<double>("maximum time step", TRANSPORT_LARGE_TIME_STEP);

  // populate the list of boundary influx functions
  bcs.clear();

  if (transport_list.isSublist("boundary conditions")) {  // New flexible format.
    std::vector<std::string> bcs_tcc_name;
    Teuchos::RCP<Teuchos::ParameterList>
       bcs_list = Teuchos::rcp(new Teuchos::ParameterList(transport_list.get<Teuchos::ParameterList>("boundary conditions")));
#ifdef ALQUIMIA_ENABLED
    TransportBCFactory bc_factory(mesh_, bcs_list, chem_state_, chem_engine_);
#else
    TransportBCFactory bc_factory(mesh_, bcs_list);
#endif
    bc_factory.CreateConcentration(bcs);

    for (int m = 0; m < bcs.size(); m++) {
      std::vector<int>& tcc_index = bcs[m]->tcc_index();
      std::vector<std::string>& tcc_names = bcs[m]->tcc_names();
      int ncomp = tcc_names.size();

      for (int i = 0; i < ncomp; i++) {
        tcc_index.push_back(FindComponentNumber(tcc_names[i]));
      }
    }
  } else {
    printf("Transport PK: does not have boundary conditions.\n");
  }

  // Create the source object if any
  srcs.clear();

  if (transport_list.isSublist("source terms")) {
    Teuchos::RCP<Teuchos::ParameterList> src_list = Teuchos::rcpFromRef(transport_list.sublist("source terms", true));
    TransportSourceFactory src_factory(mesh_, src_list);
    src_factory.CreateSource(srcs);

    for (int m = 0; m < srcs.size(); m++) {
      srcs[m]->set_tcc_index(FindComponentNumber(srcs[m]->tcc_name()));
    
      int distribution = srcs[m]->CollectActionsList();
      if (distribution & TransportActions::DOMAIN_FUNCTION_ACTION_DISTRIBUTE_PERMEABILITY) {
        Errors::Message msg;
        msg << "Transport PK: support of permeability weighted source distribution is pending.\n";
        Exceptions::amanzi_throw(msg);  
      }
    }
  }
}


/* ****************************************************************
* Find place of the given component in a multivector.
**************************************************************** */
int Transport_PK::FindComponentNumber(const std::string component_name)
{
  int ncomponents = component_names_.size();
  for (int i = 0; i < ncomponents; i++) {
    if (component_names_[i] == component_name) return i;
  } 
  return -1;
}


/* ****************************************************************
* Process string for the dispersivity model.
**************************************************************** */
void Transport_PK::ProcessStringDispersionModel(const std::string name, int* model)
{
  Errors::Message msg;
  if (name == "isotropic") {
    *model = TRANSPORT_DISPERSIVITY_MODEL_ISOTROPIC;
  } else if (name == "Bear") {
    *model = TRANSPORT_DISPERSIVITY_MODEL_BEAR;
  } else if (name == "Lichtner") {
    *model = TRANSPORT_DISPERSIVITY_MODEL_LICHTNER;
  } else {
    *model = TRANSPORT_DISPERSIVITY_MODEL_NULL;
  }
}


/* ****************************************************************
* Process string for the dispersion numerical approximation.
**************************************************************** */
void Transport_PK::ProcessStringDispersionMethod(const std::string name, int* method)
{
  Errors::Message msg;
  if (name == "two point flux approximation") {
    *method = TRANSPORT_DISPERSION_METHOD_TPFA;
  } else if (name == "nonlinear finite volume") {
    *method = TRANSPORT_DISPERSION_METHOD_NLFV;
  } else {
    *method = TRANSPORT_DISPERSION_METHOD_TPFA;
  }
}


/* ****************************************************************
* Process time integration sublist.
**************************************************************** */
void Transport_PK::ProcessStringAdvectionLimiter(const std::string name, int* method)
{
  Errors::Message msg;
  if (name == "BarthJespersen") {
    advection_limiter = TRANSPORT_LIMITER_BARTH_JESPERSEN;
  } else if (name == "Tensorial") {
    advection_limiter = TRANSPORT_LIMITER_TENSORIAL;
  } else if (name == "Kuzmin") {
    advection_limiter = TRANSPORT_LIMITER_KUZMIN;
  } else {
    msg << "Transport PK: unknown advection limiter (BarthJespersen, Tensorial, Kuzmin).\n";
    Exceptions::amanzi_throw(msg);
  }
}


/* ************************************************************* */
/* Printing information about Transport status                   */
/* ************************************************************* */
void Transport_PK::PrintStatistics() const
{
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell");

  if (vo_->getVerbLevel() > Teuchos::VERB_NONE) {
    std::cout << "Transport PK: CFL = " << cfl_ << std::endl;
    std::cout << "    Total number of components = " << tcc_prev.NumVectors() << std::endl;
    std::cout << "    Verbosity level = " << vo_->getVerbLevel() << std::endl;
    std::cout << "    Spatial/temporal discretication orders = " << spatial_disc_order
         << " " << temporal_disc_order << std::endl;
    std::cout << "    Enable internal tests = " << (internal_tests ? "yes" : "no")  << std::endl;
    std::cout << "    Advection limiter = " << (advection_limiter == TRANSPORT_LIMITER_TENSORIAL ? "Tensorial" : "BarthJespersen or Kuzmin(experimental)") << std::endl;
  }
}


/* ****************************************************************
* DEBUG: creating GMV file 
**************************************************************** */
void Transport_PK::WriteGMVfile(Teuchos::RCP<State> S) const
{
  Epetra_MultiVector& tcc_prev = *tcc->ViewComponent("cell");

  GMV::open_data_file(*mesh_, (std::string)"transport.gmv");
  GMV::start_data();
  GMV::write_cell_data(tcc_prev, 0, "component0");
  GMV::write_cell_data(*ws, 0, "saturation");
  GMV::close_data_file();
}

}  // namespace Transport
}  // namespace Amanzi

