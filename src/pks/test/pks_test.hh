#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"

#include "State.hh"
#include "EvaluatorSecondary.hh"
#include "TreeVector.hh"

#include "PK.hh"
#include "PK_Default.hh"
#include "PK_MixinLeaf.hh"
#include "PK_MixinExplicit.hh"
#include "PK_Adaptors.hh"

/*
ODE Test PKs

Test PKs:
  A: du/dt = 1     ( u = t )
  B: du/dt = u     ( u = exp(t) )
  C: du/dt = 2 * u * t  ( u = exp(t^2)  )

*/

using namespace Amanzi;

class DudtEvaluatorA : public EvaluatorSecondary<CompositeVector,CompositeVectorSpace>
{
 public:
  DudtEvaluatorA(Teuchos::ParameterList& plist) :
      EvaluatorSecondary(plist) {
    dependencies_.insert("primaryA");
  }

  DudtEvaluatorA(const DudtEvaluatorA& other) = default;
  Teuchos::RCP<Evaluator> Clone() const {
    return Teuchos::rcp(new DudtEvaluatorA(*this));
  }

 protected:
  void Evaluate_(const State& S,
                 CompositeVector& result) {
    result.PutScalar(1.);
  }

  void EvaluatePartialDerivative_(const State& S,
          const Key& wrt_key, CompositeVector& result) {
    ASSERT(wrt_key == *dependencies_.begin());
    result.PutScalar(0.);
  }
};

class DudtEvaluatorB : public EvaluatorSecondary<CompositeVector,CompositeVectorSpace>
{
 public:
  DudtEvaluatorB(Teuchos::ParameterList& plist) :
      EvaluatorSecondary(plist) {
    dependencies_.insert("primaryB");
  }

  DudtEvaluatorB(const DudtEvaluatorB& other) = default;
  Teuchos::RCP<Evaluator> Clone() const {
    return Teuchos::rcp(new DudtEvaluatorB(*this));
  }

 protected:
  void Evaluate_(const State& S,
                 CompositeVector& result) {
    result = S.Get<CompositeVector>(*dependencies_.begin(), my_tag_);
  }

  void EvaluatePartialDerivative_(const State& S,
          const Key& wrt_key, CompositeVector& result) {
    ASSERT(wrt_key == *dependencies_.begin());
    result.PutScalar(1.);
  }
};

class DudtEvaluatorC : public EvaluatorSecondary<CompositeVector,CompositeVectorSpace>
{
 public:
  DudtEvaluatorC(Teuchos::ParameterList& plist) :
      EvaluatorSecondary(plist) {
    dependencies_.insert("primaryC");
    dependencies_.insert("scaling");
  }

  DudtEvaluatorC(const DudtEvaluatorC& other) = default;
  Teuchos::RCP<Evaluator> Clone() const {
    return Teuchos::rcp(new DudtEvaluatorC(*this));
  }

 protected:
  void Evaluate_(const State& S,
                 CompositeVector& result) {
    const auto& u = S.Get<CompositeVector>("primaryC", my_tag_);
    const auto& scaling = S.Get<CompositeVector>("scaling", my_tag_);
    result.Multiply(1.0, scaling, u, 0.);
  }

  void EvaluatePartialDerivative_(const State& S,
          const Key& wrt_key, CompositeVector& result) {
    if (wrt_key == "primaryC") {
      result = S.Get<CompositeVector>("scaling", my_tag_);
    } else if (wrt_key == "scaling") {
      result = S.Get<CompositeVector>("primaryC", my_tag_);
    } else {
      ASSERT(false);
    }
  }
};
  
template<class Base_t, class Eval_t>
class PK_ODE_Explicit : public Base_t {
 public:
  PK_ODE_Explicit(const Teuchos::RCP<Teuchos::ParameterList>& pk_tree,
         const Teuchos::RCP<Teuchos::ParameterList>& global_plist,
         const Teuchos::RCP<State>& S,
         const Teuchos::RCP<TreeVectorSpace>& soln_map) :
      Base_t(pk_tree, global_plist, S, soln_map) {
    dudt_key_ = this->key_ + "_t";
  }

  void Setup(const TreeVector& soln) {
    Base_t::Setup(soln);

    this->S_->template Require<CompositeVector,CompositeVectorSpace>(this->key_, tag_old_, this->key_)
        .SetMesh(this->mesh_)
        ->SetComponent("cell",AmanziMesh::CELL,1);

    this->S_->template Require<CompositeVector,CompositeVectorSpace>(dudt_key_, this->dudt_tag_, dudt_key_)
        .SetMesh(this->mesh_)
        ->SetComponent("cell",AmanziMesh::CELL, 1);

    Teuchos::ParameterList plist(dudt_key_);
    plist.set("tag", this->dudt_tag_);
    auto dudt_eval = Teuchos::rcp(new Eval_t(plist));
    this->S_->SetEvaluator(dudt_key_, this->dudt_tag_, dudt_eval);
  }

  void Initialize() {
    Base_t::Initialize();
    this->S_->template GetW<CompositeVector>(this->key_, "", this->key_).PutScalar(1.);
    this->S_->GetRecordW(this->key_, "", this->key_).set_initialized();
  }
  
  void Functional(double t, const TreeVector& u, TreeVector& f) {
    this->S_->set_time(dudt_tag_, t);
    this->SolutionToState(u, dudt_tag_, "");
    this->SolutionToState(f, dudt_tag_, "_t");

    // this call is here because the explicit ti is not aware that it should call it
    this->ChangedSolutionPK(dudt_tag_);
    
    this->S_->GetEvaluator(dudt_key_,dudt_tag_)->Update(*this->S_, this->name());
    this->StateToSolution(f, dudt_tag_, "_t");

    std::cout << std::setprecision(16) << "  At time t = " << t << ": u = " << (*u.Data()->ViewComponent("cell",false))[0][0]
              << ", du/dt = " << (*f.Data()->ViewComponent("cell", false))[0][0] << std::endl;
  }

 protected:
  Key dudt_key_;
  using Base_t::dudt_tag_;

  using Base_t::tag_old_;
  using Base_t::tag_new_;
  
};


template<class Base_t, class Eval_t>
class PK_ODE_Implicit : public Base_t {
 public:
  PK_ODE_Implicit(const Teuchos::RCP<Teuchos::ParameterList>& pk_tree,
         const Teuchos::RCP<Teuchos::ParameterList>& global_plist,
         const Teuchos::RCP<State>& S,
         const Teuchos::RCP<TreeVectorSpace>& soln_map) :
      Base_t(pk_tree, global_plist, S, soln_map) {
    dudt_key_ = this->key_ + "_t";
  }

  void Setup(const TreeVector& soln) {
    Base_t::Setup(soln);
    
    this->S_->template Require<CompositeVector,CompositeVectorSpace>(this->key_, tag_new_, this->key_)
        .SetMesh(this->mesh_)
        ->SetComponent("cell",AmanziMesh::CELL,1);

    this->S_->template Require<CompositeVector,CompositeVectorSpace>(dudt_key_, tag_new_, dudt_key_)
        .SetMesh(this->mesh_)
        ->SetComponent("cell",AmanziMesh::CELL, 1);

    Teuchos::ParameterList plist(dudt_key_);
    plist.set("tag", tag_new_);
    auto dudt_eval = Teuchos::rcp(new Eval_t(plist));
    this->S_->SetEvaluator(dudt_key_, tag_new_, dudt_eval);
  }

  void Initialize() {
    Base_t::Initialize();
    this->S_->template GetW<CompositeVector>(this->key_, "", this->key_).PutScalar(1.);
    this->S_->GetRecordW(this->key_, this->key_).set_initialized();
  }
  
  virtual void Functional(double t_old, double t_new, Teuchos::RCP<TreeVector> u_old,
                          Teuchos::RCP<TreeVector> u_new, Teuchos::RCP<TreeVector> f) override {
    this->SolutionToState(*u_new, tag_new_, "");
    this->SolutionToState(*u_old, tag_old_, "");
    ASSERT(std::abs(t_old - S_->time(tag_old_)) < 1.e-12);
    ASSERT(std::abs(t_new - S_->time(tag_new_)) < 1.e-12);
    this->SolutionToState(*f, tag_new_, "_t");

    this->S_->GetEvaluator(dudt_key_,tag_new_)->Update(*this->S_, this->name());
    this->StateToSolution(*f, tag_new_, "_t");

    std::cout << "  At time t = " << t_new << ": u = " << (*u_new->Data()->ViewComponent("cell",false))[0][0]
              << ", du/dt = " << (*f->Data()->ViewComponent("cell", false))[0][0] << std::endl;

    double dt = t_new - t_old;
    f->Update(1.0/dt, *u_new, -1.0/dt, *u_old, -1.0);
  }
      

  virtual int ApplyPreconditioner(Teuchos::RCP<const TreeVector> u, Teuchos::RCP<TreeVector> Pu) override {
    Key deriv_key = Keys::getDerivKey(this->dudt_key_, this->key_);
    *Pu->Data() = S_->template Get<CompositeVector>(deriv_key, tag_new_);
    Pu->Data()->Shift(1.0/h_);
    Pu->ReciprocalMultiply(1.0, *Pu, *u, 0.);
    return 0;
  }

  
  virtual double ErrorNorm(Teuchos::RCP<const TreeVector> u, Teuchos::RCP<const TreeVector> du) override {
    double norm = 0.;
    du->NormInf(&norm);
    return norm;
  }

  virtual void UpdatePreconditioner(double t, Teuchos::RCP<const TreeVector> up, double h) override {
    h_ = h;
    this->SolutionToState(*up, tag_new_, "");
    this->S_->GetEvaluator(dudt_key_,tag_new_)->UpdateDerivative(*this->S_, this->name(), this->key_);
  }

  virtual bool IsAdmissible(Teuchos::RCP<const TreeVector> up) override { return true; }

  virtual bool ModifyPredictor(double h, Teuchos::RCP<const TreeVector> u0, Teuchos::RCP<TreeVector> u) override {
    return false;
  }

  virtual AmanziSolvers::FnBaseDefs::ModifyCorrectionResult 
  ModifyCorrection(double h, Teuchos::RCP<const TreeVector> res,
                   Teuchos::RCP<const TreeVector> u, Teuchos::RCP<TreeVector> du) override {
    return AmanziSolvers::FnBaseDefs::CORRECTION_NOT_MODIFIED; }

  virtual void ChangedSolution() override {
    this->ChangedSolutionPK(tag_new_);
  }

  
 protected:
  double h_;
  Key dudt_key_;

  using Base_t::tag_old_;
  using Base_t::tag_new_;
  using Base_t::S_;
};
