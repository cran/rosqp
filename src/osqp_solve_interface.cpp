

#include "osqp/include/osqp.h"
#include <Rcpp.h>
#include <memory>
#include <algorithm>
#include <vector>

using namespace Rcpp;


void extractMatrixData(const Rcpp::S4& mat, std::vector<c_int>& iout, std::vector<c_int>& pout, std::vector<c_float>& xout);
void translateSettings(OSQPSettings* settings, const Rcpp::List& pars);
void mycleanup (OSQPWorkspace* x);
S4 toDgCMat(csc*);

//typedef Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> XPtrOsqpWork;



  
  
// [[Rcpp::export]]
SEXP rosqpSetup(const S4& P, const NumericVector& q, const S4& A, const NumericVector& l, const NumericVector& u, const List& pars)
{
  
  
  
  IntegerVector dimP = P.slot("Dim");
  
  IntegerVector dimA = A.slot("Dim");
  int n = dimP[0];
  int m = dimA[0];
  if (n != dimP[1] || n != dimA[1]) stop("bug");
  std::vector<c_int> A_i, A_p, P_i, P_p;
  std::vector<c_float> A_x, P_x, qvec(q.size()), lvec(l.size()), uvec(u.size());
  
  extractMatrixData(P, P_i, P_p, P_x);
  extractMatrixData(A, A_i, A_p, A_x);
  
  std::copy(q.begin(), q.end(), qvec.begin());
  std::copy(l.begin(), l.end(), lvec.begin());
  std::copy(u.begin(), u.end(), uvec.begin());
  
  std::unique_ptr<OSQPSettings> settings (new OSQPSettings);
  set_default_settings(settings.get());
  
  if (pars.size())
    translateSettings(settings.get(), pars);
  
  std::unique_ptr<OSQPData> data (new OSQPData);
  
  data->n = static_cast<c_int>(n);
  data->m = static_cast<c_int>(m);
  data->P = csc_matrix(data->n, data->n, P_x.size(), P_x.data(), P_i.data(), P_p.data());
  data->q = qvec.data();
  data->A = csc_matrix(data->m, data->n, A_x.size(), A_x.data(), A_i.data(), A_p.data());
  data->l = lvec.data();
  data->u = uvec.data();
  
  
  Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> work(osqp_setup(data.get(), settings.get()));
  
  
  return work;
}
  
  
  

// [[Rcpp::export]]
List rosqpSolve(SEXP workPtr)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  c_int n = work->data->n;
  c_int m = work->data->m;
  c_int res = osqp_solve(work);
  
  
  
  std::string status = work->info->status;
  List info =  List::create(_("iter") = work->info->iter,
                      _("status") = status,
                      _("status_val") = work->info->status_val,
                      _("status_polish") = work->info->status_polish,
                      _("obj_val") = work->info->obj_val,
                      _("pri_res") = work->info->pri_res,
                      _("dua_res") = work->info->dua_res,
                      _("setup_time") = work->info->setup_time,
                      _("solve_time") = work->info->solve_time,
                      _("polish_time") = work->info->polish_time,
                      _("run_time") = work->info->run_time,
                      _("rho_estimate") = work->info->rho_estimate,
                      _("rho_updates") = work->info->rho_updates);
  
  List resl;
  if (res != OSQP_UNSOLVED)
  {
    NumericVector x(work->solution->x, work->solution->x + n);
    NumericVector y(work->solution->y, work->solution->y + m);
    NumericVector dx(work->delta_x, work->delta_x + n);
    NumericVector dy(work->delta_y, work->delta_y + m);
    resl = List::create(_("x") = x,
                       _("y") = y,
                       _("prim_inf_cert") = dx,
                       _("dual_inf_cert") = dy,
                       _("info") = info);
  }
  else
    resl = List::create(_("x") = NA_REAL,
                        _("y") = NA_REAL,
                        _("prim_inf_cert") = NA_REAL,
                        _("dual_inf_cert") = NA_REAL,
                        _("info") = info);
    
  return resl;
}

// [[Rcpp::export]]
List rosqpGetParams(SEXP workPtr)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  IntegerVector linsys;
  if (work->settings->linsys_solver == SUITESPARSE_LDL_SOLVER)
    linsys = IntegerVector::create(_("SUITESPARSE_LDL_SOLVER") = work->settings->linsys_solver);
  else if (work->settings->linsys_solver == MKL_PARDISO_SOLVER)
    linsys = IntegerVector::create(_("MKL_PARDISO_SOLVER") = work->settings->linsys_solver);
  else 
    linsys = IntegerVector::create(_("UNKNOWN_SOLVER") = work->settings->linsys_solver);
  
  List res = List::create(_("rho") = work->settings->rho,
                          _("sigma") = work->settings->sigma,
                          _("max_iter") = work->settings->max_iter,
                          _("eps_abs") = work->settings->eps_abs,
                          _("eps_rel") = work->settings->eps_rel,
                          _("eps_prim_inf") = work->settings->eps_prim_inf,
                          _("eps_dual_inf") = work->settings->eps_dual_inf,
                          _("alpha") = work->settings->alpha,
                          _("linsys_solver") = linsys,
                          _("delta") = work->settings->delta,
                          _("polish") = (bool)work->settings->polish,
                          _("polish_refine_iter") = work->settings->polish_refine_iter,
                          _("verbose") = (bool)work->settings->verbose,
                          _("scaled_termination") = (bool)work->settings->scaled_termination,
                          _("check_termination") = work->settings->check_termination,
                          _("warm_start") = (bool)work->settings->warm_start,
                          _("scaling") = work->settings->scaling,
                          _("adaptive_rho") = work->settings->adaptive_rho,
                          _("adaptive_rho_interval") = work->settings->adaptive_rho_interval,
                          _("adaptive_rho_tolerance") = work->settings->adaptive_rho_tolerance);
  
  res.push_back(work->settings->adaptive_rho_fraction, "adaptive_rho_fraction");
  
  return res;
}



// [[Rcpp::export]]
IntegerVector rosqpGetDims(SEXP workPtr)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  auto res = IntegerVector::create(_("n") = work->data->n,
                                   _("m") = work->data->m);
  
  
  return res;
}
// [[Rcpp::export]]
void rosqpUpdate(SEXP workPtr, Rcpp::Nullable<NumericVector> q_new, Rcpp::Nullable<NumericVector> l_new, Rcpp::Nullable<NumericVector> u_new)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  
  if (q_new.isNotNull()) {
    osqp_update_lin_cost(work, as<NumericVector>(q_new.get()).begin());
  }
  if (l_new.isNotNull()) {
    osqp_update_lower_bound(work, as<NumericVector>(l_new.get()).begin());
  }
  if (u_new.isNotNull()) {
    osqp_update_upper_bound(work, as<NumericVector>(u_new.get()).begin());
  }
}

void extractMatrixData(const S4& mat, std::vector<c_int>& iout, std::vector<c_int>& pout, std::vector<c_float>& xout)
{
  IntegerVector i = mat.slot("i");
  IntegerVector p = mat.slot("p");
  NumericVector x = mat.slot("x");
  
  iout.resize(i.size());
  pout.resize(p.size());
  xout.resize(x.size());
  std::copy(i.begin(), i.end(), iout.begin());
  std::copy(p.begin(), p.end(), pout.begin());
  std::copy(x.begin(), x.end(), xout.begin());
  
  return;
}


void translateSettings(OSQPSettings* settings, const List& pars)
{
  
  CharacterVector nms(pars.names());
  for (int i = 0; i < pars.size(); i++)
  {
    if (Rf_isNull(nms[i]))
      continue;
    auto nm = as<std::string>(nms[i]);
    
    if (nm == "rho")
      settings->rho = as<c_float>(pars[i]);
    else if (nm == "sigma")
      settings->sigma = as<c_float>(pars[i]);
    else if (nm == "eps_abs")
      settings->eps_abs = as<c_float>(pars[i]);
    else if (nm == "eps_rel")
      settings->eps_rel = as<c_float>(pars[i]);
    else if (nm == "eps_prim_inf")
      settings->eps_prim_inf = as<c_float>(pars[i]);
    else if (nm == "eps_dual_inf")
      settings->eps_dual_inf = as<c_float>(pars[i]);
    else if (nm == "alpha")
      settings->alpha = as<c_float>(pars[i]);
    else if (nm == "delta")
      settings->delta = as<c_float>(pars[i]);
    else if (nm == "adaptive_rho_fraction")
      settings->adaptive_rho_fraction = as<c_float>(pars[i]);
    else if (nm == "adaptive_rho_tolerance")
      settings->adaptive_rho_tolerance = as<c_float>(pars[i]);
    
    
    else if (nm == "linsys_solver")
      settings->linsys_solver = (linsys_solver_type)as<c_int>(pars[i]);
    else if (nm == "polish_refine_iter")
      settings->polish_refine_iter = as<c_int>(pars[i]);
    else if (nm == "check_termination")
      settings->check_termination = as<c_int>(pars[i]);
    else if (nm == "scaling")
      settings->scaling = as<c_int>(pars[i]);
    else if (nm == "max_iter")
      settings->max_iter = as<c_int>(pars[i]);
    else if (nm == "adaptive_rho")
      settings->adaptive_rho = as<c_int>(pars[i]);
    else if (nm == "adaptive_rho_interval")
      settings->adaptive_rho_interval = as<c_int>(pars[i]);
    
    
    else if (nm == "polish")
      settings->polish = as<c_int>(pars[i]);
    else if (nm == "verbose")
      settings->verbose = as<c_int>(pars[i]);
    else if (nm == "scaled_termination")
      settings->scaled_termination = as<c_int>(pars[i]);
    else if (nm == "warm_start")
      settings->warm_start = as<c_int>(pars[i]);
  }
  
  return;
}

// [[Rcpp::export]]
void rosqpUpdateSettings(SEXP workPtr, SEXP val, std::string nm)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  
  if (nm == "check_termination")
    osqp_update_check_termination(work, as<c_int>(val));
  else if (nm == "max_iter")
    osqp_update_max_iter(work, as<c_int>(val));
  else if (nm == "polish")
    osqp_update_polish(work, as<c_int>(val));
  else if (nm == "polish_refine_iter")
    osqp_update_polish_refine_iter(work, as<c_int>(val));
  else if (nm == "rho")
    osqp_update_rho(work, as<c_float>(val));
  else if (nm == "scaled_termination")
    osqp_update_scaled_termination(work, as<c_int>(val));
  else if (nm == "verbose")
    osqp_update_verbose(work, as<c_int>(val));
  else if (nm == "warm_start")
    osqp_update_warm_start(work, as<c_int>(val));
  else if (nm == "alpha")
    osqp_update_alpha(work, as<c_float>(val));
  else if (nm == "delta")
    osqp_update_delta(work, as<c_float>(val));
  else if (nm == "eps_abs")
    osqp_update_eps_abs(work, as<c_float>(val));
  else if (nm == "eps_dual_inf")
    osqp_update_eps_dual_inf(work, as<c_float>(val));
  else if (nm == "eps_prim_inf")
    osqp_update_eps_prim_inf(work, as<c_float>(val));
  else if (nm == "eps_rel")
    osqp_update_eps_rel(work, as<c_float>(val));
  else
    Rcout << "Param " + nm + " cannot be updated live" << std::endl;
  
}

// [[Rcpp::export]]
SEXP rosqpGetData(SEXP workPtr, std::string nm)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  
  if (nm == "P")
    return toDgCMat(work->data->P);
  if (nm == "A")
    return toDgCMat(work->data->A);
  
  if (nm == "q")
  {
    int n = work->data->n;
    NumericVector q(work->data->q, work->data->q + n);
    return q;
  }
  if (nm == "l")
  {
    int n = work->data->m;
    NumericVector q(work->data->l, work->data->l + n);
    return q;
  }
  if (nm == "u")
  {
    int n = work->data->m;
    NumericVector q(work->data->u, work->data->u + n);
    return q;
  }
    
  
  return R_NilValue;
}

S4 toDgCMat(csc* inmat)
{
  S4 m("dgCMatrix");
  
  int nnz = inmat->nzmax;
  int nr = inmat->m;
  int nc = inmat->n;
  
  NumericVector x(inmat->x, inmat->x+nnz);
  IntegerVector i(inmat->i, inmat->i+nnz);
  IntegerVector p(inmat->p, inmat->p+nc+1);
  IntegerVector dim = IntegerVector::create(nr, nc);
  
  
  m.slot("i")   = i;
  m.slot("p")   = p;
  m.slot("x")   = x;
  m.slot("Dim") = dim;
  
  return m;
}


void mycleanup (OSQPWorkspace* x)
{
  osqp_cleanup(x);
}


// [[Rcpp::export]]
void rosqpWarmStart(SEXP workPtr, Rcpp::Nullable<NumericVector> x, Rcpp::Nullable<NumericVector> y)
{
  auto work = as<Rcpp::XPtr<OSQPWorkspace, Rcpp::PreserveStorage, mycleanup> >(workPtr);
  
  if(x.isNull() && y.isNull()) 
  {
    return;
  } else if (x.isNotNull() && y.isNotNull())
  {
    osqp_warm_start(work, as<NumericVector>(x.get()).begin(),as<NumericVector>(y.get()).begin());
    
    
  } else if (x.isNotNull())
  {
    osqp_warm_start_x(work, as<NumericVector>(x.get()).begin());
  } else {
    osqp_warm_start_y(work, as<NumericVector>(y.get()).begin());
  }
  return;
}
