% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/osqp.R
\name{osqp}
\alias{osqp}
\title{OSQP Solver object}
\usage{
osqp(P = NULL, q = NULL, A = NULL, l = NULL, u = NULL,
  pars = osqpSettings())
}
\arguments{
\item{P, A}{sparse matrices of class dgCMatrix or coercible into such, with P positive semidefinite.}

\item{q, l, u}{Numeric vectors, with possibly infinite elements in l and u}

\item{pars}{list with optimization parameters, conveniently set with the function 
\code{\link{osqpSettings}}. For \code{osqpObject$UpdateSettings(newPars)} only a subset of the settings 
can be updated once the problem has been initnialized.}
}
\value{
An R6-object of class "rosqp_model" with methods defined which can be further 
used to solve the problem with updated settings / parameters.
}
\description{
OSQP Solver object
}
\details{
Allows one to solve a parametric 
problem with for example warm starts between updates of the parameter, c.f. the examples.
The object returned by \code{osqp} contains several methods which can be used to either update/get details of the 
problem, modify the optimization settings or attempt to solve the problem.
}
\section{Usage}{

\preformatted{model = osqp(P=NULL, q=NULL, A=NULL, l=NULL, u=NULL, pars=osqpSettings())

model$Solve()
model$Update(q = NULL, l = NULL, u = NULL)
model$GetParams()
model$GetDims()
model$UpdateSettings(newPars = list())

model$GetData(element = c("P", "q", "A", "l", "u"))
model$WarmStart(x=NULL, y=NULL)

print(model)
}
}

\section{Method Arguments}{

\describe{
  \item{element}{a string with the name of one of the matrices / vectors of the problem}
  \item{newPars}{list with optimization parameters}
}
}

\examples{
## example, adapted from the osqp documentation 
\dontrun{
library(rosqp)
library(Matrix)
set.seed(1)
n = 10
m = 1000
Ad = matrix(0, m, n)
Ad[sample(n*m, n*m/2, FALSE)] = runif(n*m/2)
x_true = (runif(n) > 0.8) * runif(n) / sqrt(n)
b = drop(Ad \%*\% x_true) + 0.5 * runif(m)
gammas = seq(1, 10, length.out = 11)

# \% OSQP data
P = .sparseDiagonal(2*n+m, c(numeric(n), rep_len(1, m), numeric(n)))
q = numeric(2*n+m);
A = rbind(cbind(Ad, 
                -Diagonal(m), 
                sparseMatrix(numeric(), numeric(), x=numeric(), dims=c(m, n))),
          cbind(Diagonal(n), 
                sparseMatrix(numeric(), numeric(), x=numeric(), dims=c(n, m)), 
                -Diagonal(n)),
          cbind(Diagonal(n), 
                sparseMatrix(numeric(), numeric(), x=numeric(), dims=c(n, m)), 
                Diagonal(n))
          )
l = c(b, rep_len(-Inf, n), numeric(n))
u = c(b, numeric(n), rep_len(Inf, n))

model = osqp(P, q, A, l, u, osqpSettings(verbose = FALSE))

res = sapply(gammas, function(gamma) {
  q_new = c(numeric(n+m), rep_len(gamma, n))
  model$Update(q=q_new)
  res = model$Solve()
  res$x
})
}
}
\seealso{
\code{\link{solve_osqp}}
}
