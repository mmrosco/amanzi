/*
  Copyright 2010-201x held jointly by participating institutions.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:

*/

//!

#include <iostream>
#include "UnitTest++.h"

#include "Teuchos_RCP.hpp"
#include "AmanziVector.hh"
#include "AmanziMatrix.hh"

#include "AmanziComm.hh"
#include "AmanziVector.hh"
#include "exceptions.hh"
#include "LinearOperatorFactory.hh"
#include "LinearOperatorPCG.hh"
#include "LinearOperatorGMRES.hh"
#include "LinearOperatorNKA.hh"
// #include "LinearOperatorBelosGMRES.hh"
// #include "LinearOperatorAmesos.hh"

using namespace Amanzi;

SUITE(SOLVERS)
{
  class Matrix {
    using CrsMatrix_type = Tpetra::CrsMatrix<double, int, int>;

   public:
    Matrix(){};
    Matrix(const Map_ptr_type& map) : map_(map)
    {
      x_[0] = 0.00699270335645641;
      x_[1] = 0.01398540671291281;
      x_[2] = 0.02079636439636044;
      x_[3] = 0.02688033938777295;
      x_[4] = 0.03122225970045909;
    };
    ~Matrix(){};
    Matrix(const Matrix& other) : map_(other.map_){};

    Teuchos::RCP<Matrix> Clone() const
    {
      return Teuchos::rcp(new Matrix(*this));
    }

    // 5-point FD stencil
    virtual int apply(const Vector_type& v, Vector_type& mv) const
    {
      int N = v.getMap()->getNodeNumElements();
      int n = std::pow(N, 0.5);

      std::array<double, 5> coefs{ -1., -1., 4., -1., -1. };
      auto vv = v.getLocalViewDevice();
      auto mvv = mv.getLocalViewDevice();

      typedef Kokkos::TeamPolicy<>::member_type MemberType;
      // Create an instance of the policy
      Kokkos::TeamPolicy<> policy(N, Kokkos::AUTO());
      Kokkos::parallel_for(policy, KOKKOS_LAMBDA(MemberType team) {
        int k = team.league_rank();
        int i = k % n;
        int j = k / n;

        std::array<int, 5> inds{ j > 0 ? k - n : -1,
                                 i > 0 ? k - 1 : -1,
                                 k,
                                 i < n - 1 ? k + 1 : -1,
                                 j < n - 1 ? k + n : -1 };
        double sum = 0.;
        Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, 5),
                                KOKKOS_LAMBDA(int i, double& lsum) {
                                  int c = inds[i];
                                  lsum += c < 0 ? 0. : coefs[i] * vv(c, 0);
                                },
                                sum);
        mvv(k, 0) = sum;
      });
      return 0;
    }

    virtual int applyInverse(const Vector_type& v, Vector_type& hv) const
    {
      auto vv = v.getLocalViewDevice();
      auto hvv = hv.getLocalViewDevice();

      Kokkos::parallel_for(hvv.extent(0),
                           KOKKOS_LAMBDA(int i) { hvv(i, 0) = vv(i, 0); });
      return 0;
    }

    // 3-point FD stencil
    void Init()
    {
      // int n = map_->getNodeNumElements();
      // A_ = Teuchos::rcp(new CrsMatrix_type(map_, map_, 3));
      // for (int i = 0; i < n; i++) {
      //   int indices[3];
      //   double values[3] = {double(-i), double(2 * i + 1), double(-i - 1)};
      //   for (int k = 0; k < 3; k++) indices[k] = i + k - 1;
      //   A_->insertLocalValues(i, 3, values, indices);
      // }
      // A_->fillComplete(map_, map_);
    }

    // // partial consistency with Operators'interface
    // Teuchos::RCP<CrsMatrix_type> A() const { return A_; }
    // void CopyVectorToSuperVector(const Vector_type& ev, Vector_type& sv)
    // const { sv = ev; } void CopySuperVectorToVector(const Vector_type& sv,
    // Vector_type& ev) const { ev = sv; }

    const Map_ptr_type& getDomainMap() const { return map_; }
    const Map_ptr_type& getRangeMap() const { return map_; }
    double* x() { return x_; }

   private:
    Map_ptr_type map_;
    double x_[5];
    //  Teuchos::RCP<CrsMatrix_type> A_;
  };


  TEST(PCG_SOLVER)
  {
    std::cout << "Checking PCG solver..." << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    // create the pcg operator
    Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
    AmanziSolvers::LinearOperatorPCG<Matrix, Vector_type, Map_type> pcg(m, m);
    pcg.Init();

    // initial guess
    Vector_type u(map);
    {
      auto uv = u.getLocalViewHost();
      uv(55, 0) = 1.0;
    }
    u.sync_device();

    // solve
    Vector_type v(map);
    int ierr = pcg.applyInverse(u, v);
    CHECK(ierr > 0);
    CHECK_EQUAL(28, pcg.num_itrs());

    v.sync_host();
    {
      auto vv = v.getLocalViewHost();
      for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
    }
  };

  TEST(GMRES_SOLVER_LEFT_PRECONDITIONER)
  {
    std::cout << "\nChecking GMRES solver with LEFT preconditioner..."
              << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    // create the gmres operator
    std::vector<int> nits = { 73, 61 };
    for (int loop = 0; loop < 2; loop++) {
      Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
      AmanziSolvers::LinearOperatorGMRES<Matrix, Vector_type, Map_type> gmres(
        m, m);
      gmres.Init();
      gmres.set_krylov_dim(15 + loop * 5);
      gmres.set_tolerance(1e-12);

      // initial guess
      Vector_type u(map);
      {
        auto uv = u.getLocalViewHost();
        uv(55, 0) = 1.0;
      }
      u.sync_device();

      // solve
      Vector_type v(map);
      int ierr = gmres.applyInverse(u, v);
      CHECK(ierr > 0);
      CHECK_EQUAL(nits[loop], gmres.num_itrs());
      v.sync_host();
      {
        auto vv = v.getLocalViewHost();
        for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
      }
    }
  };

  TEST(GMRES_SOLVER_RIGHT_PRECONDITIONER)
  {
    std::cout << "\nChecking GMRES solver with RIGHT preconditioner..."
              << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    Teuchos::ParameterList plist;
    plist.set<std::string>("preconditioning strategy", "right");

    // create the gmres operator
    std::vector<int> nits = { 73, 61 };
    for (int loop = 0; loop < 2; loop++) {
      Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
      AmanziSolvers::LinearOperatorGMRES<Matrix, Vector_type, Map_type> gmres(
        m, m);
      gmres.Init(plist);
      gmres.set_krylov_dim(15 + loop * 5);
      gmres.set_tolerance(1e-12);

      // initial guess
      Vector_type u(map);
      {
        auto uv = u.getLocalViewHost();
        uv(55, 0) = 1.0;
      }
      u.sync_device();

      // solve
      Vector_type v(map);
      int ierr = gmres.applyInverse(u, v);
      CHECK(ierr > 0);
      CHECK_EQUAL(nits[loop], gmres.num_itrs());
      v.sync_host();
      {
        auto vv = v.getLocalViewHost();
        for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
      }
    }
  };

  TEST(GMRES_SOLVER_DEFLATION)
  {
    std::cout << "\nChecking GMRES solver with deflated restart..."
              << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    Teuchos::ParameterList plist;
    plist.set<int>("maximum size of deflation space", 5);
    plist.set<int>("maximum number of iterations", 200);
    plist.set<int>("size of Krylov space", 15);
    plist.set<double>("error tolerance", 1e-12);
    Teuchos::ParameterList& vlist = plist.sublist("verbose object");
    vlist.set("verbosity level", "extreme");

    // create the gmres operator
    Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
    AmanziSolvers::LinearOperatorGMRES<Matrix, Vector_type, Map_type> gmres(m,
                                                                            m);
    gmres.Init(plist);

    // initial guess
    Vector_type u(map);
    {
      auto uv = u.getLocalViewHost();
      uv(55, 0) = 1.0;
    }
    u.sync_device();

    // solve
    Vector_type v(map);
    int ierr = gmres.applyInverse(u, v);
    CHECK(ierr > 0);
    CHECK_EQUAL(60, gmres.num_itrs());
    v.sync_host();
    {
      auto vv = v.getLocalViewHost();
      for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
    }
  };

  TEST(NKA_SOLVER)
  {
    std::cout << "\nChecking NKA solver..." << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    // create the pcg operator
    Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
    AmanziSolvers::LinearOperatorNKA<Matrix, Vector_type, Map_type> nka(m, m);
    nka.Init();

    // initial guess
    Vector_type u(map);
    {
      auto uv = u.getLocalViewHost();
      uv(55, 0) = 1.0;
    }
    u.sync_device();

    // solve
    Vector_type v(map);
    int ierr = nka.applyInverse(u, v);
    CHECK(ierr > 0);
    CHECK_EQUAL(62, nka.num_itrs());
    v.sync_host();
    {
      auto vv = v.getLocalViewHost();
      for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
    }
  };

  // TEST(BELOS_GMRES_SOLVER) {
  //   std::cout << "\nChecking Belos GMRES solver..." << std::endl;

  //   Teuchos::ParameterList plist;
  //   Teuchos::ParameterList& vo = plist.sublist("VerboseObject");
  //   vo.set("Verbosity Level", "high");
  //   plist.set<int>("size of Krylov space", 15);
  //   plist.set<double>("error tolerance", 1e-12);

  //   auto comm = getDefaultComm();
  //   auto map = Teuchos::rcp(new Map_type(100, 0, comm));

  //   // create the operator
  //   Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
  //   AmanziSolvers::LinearOperatorBelosGMRES<Matrix, Vector_type, Map_type>
  //   gmres(m, m); gmres.Init(plist);

  //   // initial guess
  //   Vector_type u(map);
  //  {
  //    auto uv = u.getLocalViewHost();
  //    uv(55,0) = 1.0;
  //  }
  //  u.sync_device();

  //   // solve
  //   Vector_type v(map);
  //   int ierr = gmres.applyInverse(u, v);
  //   CHECK(ierr > 0);
  // v.sync_host();
  // {
  //   auto vv = v.getLocalViewHost();
  //   for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i,0), 1e-6);
  // }


  //
  // };

  // TEST(AMESOS_SOLVER) {
  //   std::cout << "\nChecking Amesos solver..." << std::endl;

  //   Teuchos::ParameterList plist;
  //   Teuchos::ParameterList& vo = plist.sublist("VerboseObject");
  //   vo.set("Verbosity Level", "high");
  //   plist.set<std::string>("solver name", "Amesos_Klu")
  //        .set<int>("amesos version", 1);

  //   auto comm = getDefaultComm();
  //   auto map = Teuchos::rcp(new Map_type(10, 0, comm));

  //   // create the operator
  //   Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
  //   m->Init();

  //   // initial guess
  //   Vector_type v(map), u(map);
  //  {
  //    auto uv = u.getLocalViewHost();
  //    uv(55,0) = 1.0;
  //  }
  //  u.sync_device();

  //   // Amesos1
  //   {
  //     AmanziSolvers::LinearOperatorAmesos<Matrix, Vector_type, Map_type>
  //     klu(m, m); klu.Init(plist);

  //     int ierr = klu.applyInverse(u, v);
  //     CHECK(ierr > 0);

  //     double residual = 11 * v[5] - 5 * v[4] - 6 * v[6];
  //     CHECK_CLOSE(residual, 1.0, 1e-12);
  //   }

  //   // Amesos2
  //   {
  //     AmanziSolvers::LinearOperatorAmesos<Matrix, Vector_type, Map_type>
  //     klu(m, m); plist.set<std::string>("solver name", "Klu2")
  //          .set<int>("amesos version", 2);
  //     klu.Init(plist);

  //     int ierr = klu.applyInverse(u, v);
  //     CHECK(ierr > 0);

  //     double residual = 11 * v[5] - 5 * v[4] - 6 * v[6];
  //     CHECK_CLOSE(residual, 1.0, 1e-12);
  //   }

  //
  // };

  TEST(SOLVER_FACTORY)
  {
    std::cout << "\nChecking solver factory..." << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    Teuchos::ParameterList plist;
    Teuchos::ParameterList& slist = plist.sublist("pcg");
    slist.set<std::string>("iterative method", "pcg");

    // create the pcg operator
    Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
    AmanziSolvers::LinearOperatorFactory<Matrix, Vector_type, Map_type> factory;
    Teuchos::RCP<AmanziSolvers::LinearOperator<Matrix, Vector_type, Map_type>>
      solver = factory.Create("pcg", plist, m);
    solver->Init();

    // initial guess
    Vector_type u(map);
    {
      auto uv = u.getLocalViewHost();
      uv(55, 0) = 1.0;
    }
    u.sync_device();

    // solve
    Vector_type v(map);
    int ierr = solver->applyInverse(u, v);
    CHECK(ierr > 0);
    v.sync_host();
    {
      auto vv = v.getLocalViewHost();
      for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
    }
  };

  TEST(VERBOSITY_OBJECT)
  {
    std::cout << "\nChecking verbosity object..." << std::endl;

    auto comm = getDefaultComm();
    auto map = Teuchos::rcp(new Map_type(100, 0, comm));

    Teuchos::ParameterList plist;
    Teuchos::ParameterList& slist = plist.sublist("gmres");
    slist.set<std::string>("iterative method", "gmres");
    slist.sublist("gmres parameters")
      .set("size of Krylov space", 50)
      .set<double>("error tolerance", 1e-12);
    slist.sublist("gmres parameters")
      .sublist("verbose object")
      .set("verbosity level", "extreme");

    // create the pcg operator
    Teuchos::RCP<Matrix> m = Teuchos::rcp(new Matrix(map));
    AmanziSolvers::LinearOperatorFactory<Matrix, Vector_type, Map_type> factory;
    Teuchos::RCP<AmanziSolvers::LinearOperator<Matrix, Vector_type, Map_type>>
      solver = factory.Create("gmres", plist, m);

    // initial guess
    Vector_type u(map);
    {
      auto uv = u.getLocalViewHost();
      uv(55, 0) = 1.0;
    }
    u.sync_device();

    // solve
    Vector_type v(map);
    int ierr = solver->applyInverse(u, v);
    CHECK(ierr > 0);
    v.sync_host();
    {
      auto vv = v.getLocalViewHost();
      for (int i = 0; i < 5; i++) CHECK_CLOSE((m->x())[i], vv(i, 0), 1e-6);
    }
  };

} // suite SOLVERS